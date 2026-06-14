#include "cgi/CgiHandler.hpp"
#include "log/log.hpp"
#include "http/HttpResponse.hpp"

CgiHandler::CgiHandler()
	: pid(-1),
	  pipeStdin(-1),
	  pipeStdout(-1),
	  clientFd(-1),
	  output(""),
	  serverConfig(NULL),
	  isActive(false)
{
}

CgiHandler::CgiHandler(const CgiHandler & that)
	: pid(that.pid),
	  pipeStdin(that.pipeStdin),
	  pipeStdout(that.pipeStdout),
	  clientFd(that.clientFd),
	  output(that.output),
	  serverConfig(that.serverConfig),
	  isActive(that.isActive)
{
}

CgiHandler::~CgiHandler()
{
	cleanup();
}

CgiHandler & CgiHandler::operator=(const CgiHandler & that)
{
	if (this != &that)
	{
		cleanup();
		pid = that.pid;
		pipeStdin = that.pipeStdin;
		pipeStdout = that.pipeStdout;
		clientFd = that.clientFd;
		output = that.output;
		serverConfig = that.serverConfig;
		isActive = that.isActive;
	}
	return (*this);
}

std::map<std::string, std::string>	CgiHandler::buildEnv(const Client & client, const RequestContext & context, const std::string & scriptPath) const
{
	std::map<std::string, std::string>	env;
	std::ostringstream					contentLengthStream;
	std::string							queryString;
	std::string							pathInfo;
	std::string							scriptName;
	std::string							requestPath;
	size_t								questionPos;
	size_t								scriptPos;

	requestPath = client.getPath();
	questionPos = requestPath.find('?');
	if (questionPos != std::string::npos)
	{
		queryString = requestPath.substr(questionPos + 1);
		requestPath = requestPath.substr(0, questionPos);
	}
	scriptPos = requestPath.find(context.getMatchedLocation()->getPath());
	if (scriptPos == 0)
	{
		scriptName = requestPath;
		pathInfo = "";
	}
	else
	{
		scriptName = context.getMatchedLocation()->getPath();
		pathInfo = requestPath.substr(scriptName.length());
	}
	env[WebServ::CGI_REQUEST_METHOD] = client.getMethod();
	env[WebServ::CGI_QUERY_STRING] = queryString;
	env[WebServ::CGI_PATH_INFO] = pathInfo;
	env[WebServ::CGI_SCRIPT_NAME] = scriptName;
	env[WebServ::CGI_SCRIPT_FILENAME] = scriptPath;
	env[WebServ::CGI_SERVER_PROTOCOL] = client.getVersion();
	env[WebServ::CGI_SERVER_SOFTWARE] = WebServ::SERVER_NAME;
	env[WebServ::CGI_SERVER_NAME] = context.getTargetServer()->getServerName();
	if (env[WebServ::CGI_SERVER_NAME].empty())
		env[WebServ::CGI_SERVER_NAME] = WebServ::SERVER_NAME;
	env[WebServ::CGI_GATEWAY_INTERFACE] = WebServ::GATEWAY_INT;
	env[WebServ::CGI_REDIRECT_STATUS] = WebServ::PHP_REDIRECT;
	contentLengthStream << client.getUnchunkedBody().size();
	env[WebServ::CGI_CONTENT_LENGTH] = contentLengthStream.str();
	if (client.getHeaders().find("Content-Type") != client.getHeaders().end())
		env[WebServ::CGI_CONTENT_TYPE] = client.getHeaders().find("Content-Type")->second;
	else
		env[WebServ::CGI_CONTENT_TYPE] = "";
	return (env);
}

std::string	CgiHandler::extractScriptPath(const RequestContext & context) const
{
	std::string	remainingPath;
	std::string	locationPath;
	std::string	requestPath;
	std::string	scriptPath;
	std::string	root;

	requestPath = context.getRequestPath();
	locationPath = context.getMatchedLocation()->getPath();
	root = context.getTargetServer()->getRoot();
	if (context.hasCustomRoot())
		root = context.getMatchedLocation()->getRoot();
	if (requestPath.find(locationPath) == 0)
	{
		remainingPath = requestPath.substr(locationPath.length());
		if (remainingPath.empty() || remainingPath[0] != '/')
			remainingPath = "/" + remainingPath;
	}
	else
		remainingPath = requestPath;
	if (root[root.length() - 1] == '/')
		scriptPath = root;
	else
		scriptPath = root + "/";
	if (!remainingPath.empty() && remainingPath[0] == '/')
		remainingPath = remainingPath.substr(1);
	scriptPath += remainingPath;
	return (scriptPath);
}

std::string	CgiHandler::findInterpreter(const std::string & scriptPath) const
{
	std::string	extension;
	size_t		dotPos;

	dotPos = scriptPath.rfind('.');
	if (dotPos == std::string::npos)
		return ("");
	extension = scriptPath.substr(dotPos);
	if (extension == WebServ::EXTENSION_PHP)
		return (WebServ::INTERPRETER_PHP);
	if (extension == WebServ::EXTENSION_PYTHON)
		return (WebServ::INTERPRETER_PYTHON);
	return ("");
}

std::string	CgiHandler::parseCgiOutput(const std::string & output, HttpResponse & response) const
{
	std::ostringstream	stream;
	std::string			normalized;
	std::string			value;
	std::string			body;
	std::string			line;
	std::string			key;
	size_t				expectedLength;
	size_t				headerEnd;
	size_t				colonPos;
	size_t				lineEnd;
	size_t				pos;
	size_t				i;
	bool				hasContentLength;
	bool				hasStatus;

	log(std::string("CGI output is being parsed:\n") + output);
	normalized = output;
	if (normalized.find(WebServ::CRLF + WebServ::CRLF) == std::string::npos)
	{
		if (normalized.find("\n\n") != std::string::npos)
		{
			log("CGI output ends lines with double line feeds instead of CRLF");
			i = 0;
			while (i < normalized.length())
			{
					if (normalized[i] == '\n' && (i == 0 || normalized[i - 1] != '\r'))
					{
							normalized.insert(i, "\r");
							++i;
					}
					++i;
			}
		}
	}
	headerEnd = normalized.find(WebServ::CRLF + WebServ::CRLF);
	if (headerEnd == std::string::npos)
	{
		log("CGI headers not found");
		log("assigning status code 200 and Content-Type text/html");
		log("assigning CGI output to response body");
		response.setStatus(200);
		response.setHeader("Content-Type", "text/html");
		response.setBody(output);
		return (output);
	}
	else
		log("CGI headers found");
	pos = 0;
	hasContentLength = false;
	expectedLength = 0;
	while (pos < headerEnd)
	{
		lineEnd = normalized.find(WebServ::CRLF, pos);
		if (lineEnd == std::string::npos)
			break;
		line = normalized.substr(pos, lineEnd - pos);
		if (line.empty())
		{
			pos = lineEnd + 2;
			continue;
		}
		colonPos = line.find(':');
		if (colonPos == std::string::npos)
		{
			log(std::string("found malformed header:") + line);
			pos = lineEnd + 2;
			continue;
		}
		key = line.substr(0, colonPos);
		value = line.substr(colonPos + 1);
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
			value.erase(0, 1);
		stream.str("");
		stream.clear();
		stream << "parsing header " << key << ": " << value;
		log(stream.str());
		if (key == "Status")
		{
			hasStatus = true;
			response.setStatus(std::atoi(value.c_str()));
		}
		else if (key == "Content-Length")
		{
			hasContentLength = true;
			expectedLength = std::atoi(value.c_str());
			response.setHeader(key, value);
		}
		else
			response.setHeader(key, value);
		pos = lineEnd + 2;
	}
	body = normalized.substr(headerEnd + 4);
	if (hasContentLength)
	{
		response.setRawBody(body);
		if (body.size() != expectedLength)
		{
			stream.str("");
			stream.clear();
			stream << "CGI body size " << body.size()
					<< " does not match Content-Length " << expectedLength;
			logError(stream.str());
		}
	}
	else
		response.setBody(body);
	if (!hasStatus)
		response.setStatus(200);
	return (body);
}

void	CgiHandler::cleanup()
{
	if (pipeStdout != -1)
	{
		close(pipeStdout);
		pipeStdout = -1;
	}
	if (pipeStdin != -1)
	{
		close(pipeStdin);
		pipeStdin = -1;
	}
	if (pid != -1)
	{
		kill(pid, SIGTERM);
		pid = -1;
	}
	output.clear();
	clientFd = -1;
	serverConfig = NULL;
	isActive = false;
}

void	CgiHandler::startExecution(int clientFd, const Client & client, const RequestContext & context, std::vector<struct pollfd> & pollFds, std::map<int, int> & clientToPipe, std::map<int, const ServerConfig *> & pipeToServer)
{
	std::map<std::string, std::string>::const_iterator	envIt;
	std::map<std::string, std::string>					env;
	std::vector<std::string>							envStrings;
	std::vector<char *>									envPtrs;
	std::ostringstream									stream;
	struct pollfd										newPollFd;
	std::string											scriptPath;
	std::string											interpreter;
	size_t												envIndex;
	pid_t												pid;
	char **												envArray;
	char *												argv[3];
	int													pipeStdin[2];
	int													pipeStdout[2];
	int													stdinWriteFd;
	int													stdoutReadFd;

	if (isActive)
	{
		logError("CGI handler is already active");
		return;
	}
	scriptPath = extractScriptPath(context);
	interpreter = findInterpreter(scriptPath);
	if (interpreter.empty())
	{
		logError(std::string("no interpreter found for ") + scriptPath);
		return;
	}
	if (pipe(pipeStdin) == -1 || pipe(pipeStdout) == -1)
	{
		logError("pipe() failed");
		return;
	}
	pid = fork();
	if (pid == -1)
	{
		logError("fork() failed");
		close(pipeStdin[0]);
		close(pipeStdin[1]);
		close(pipeStdout[0]);
		close(pipeStdout[1]);
		return;
	}
	if (pid == 0)
	{
		stream << "child is executing CGI script " << scriptPath << " with " << interpreter;
		log(stream.str());
		close(pipeStdin[1]);
		close(pipeStdout[0]);
		dup2(pipeStdin[0], STDIN_FILENO);
		dup2(pipeStdout[1], STDOUT_FILENO);
		close(pipeStdin[0]);
		close(pipeStdout[1]);
		env = buildEnv(client, context, scriptPath);
		envStrings.clear();
		envIt = env.begin();
		while (envIt != env.end())
		{
			envStrings.push_back(envIt->first + "=" + envIt->second);
			++envIt;
		}
		envPtrs.reserve(envStrings.size() + 1);
		envIndex = 0;
		while (envIndex < envStrings.size())
		{
			envPtrs.push_back(const_cast<char *>(envStrings[envIndex].c_str()));
			++envIndex;
		}
		envPtrs.push_back(NULL);
		envArray = &envPtrs[0];
		argv[0] = const_cast<char *>(interpreter.c_str());
		argv[1] = const_cast<char *>(scriptPath.c_str());
		argv[2] = NULL;
		execve(argv[0], argv, envArray);
		logError(std::string("execve() failed"));
		std::exit(EXIT_FAILURE);
	}
	log("parent is writing child output to write-end of pipe");
	close(pipeStdin[0]);
	close(pipeStdout[1]);
	stdinWriteFd = pipeStdin[1];
	stdoutReadFd = pipeStdout[0];
	write(stdinWriteFd, client.getUnchunkedBody().c_str(), client.getUnchunkedBody().size());
	close(stdinWriteFd);
	log("parent is adding read-end of non-blocking pipe to FD list of poll()");
	fcntl(stdoutReadFd, F_SETFL, O_NONBLOCK);
	this->pid = pid;
	this->pipeStdin = -1;
	this->pipeStdout = stdoutReadFd;
	this->clientFd = clientFd;
	this->serverConfig = context.getTargetServer();
	this->isActive = true;
	pipeToServer[stdoutReadFd] = this->serverConfig;
	newPollFd.fd = stdoutReadFd;
	newPollFd.events = POLLIN;
	newPollFd.revents = 0;
	pollFds.push_back(newPollFd);
	clientToPipe[clientFd] = stdoutReadFd;
	stream.str("");
	stream.clear();
	stream << "CGI is ready for client " << clientFd << " pipe " << stdoutReadFd;
	log(stream.str());
}

int	CgiHandler::handlePipeOutput(int pipeFd, std::map<int, std::string> & pendingResponses, std::vector<struct pollfd> & pollFds, std::map<int, int> & clientToPipe, std::map<int, const ServerConfig *> & pipeToServer)
{
	std::ostringstream	stream;
	HttpResponse		response;
	ssize_t				bytesRead;
	size_t				i;
	pid_t				waitResult;
	char				buffer[WebServ::CGI_BUFFER];
	int					status;
	int					pipeToRemove;

	if (!isActive || pipeStdout != pipeFd)
		return (-1);
	stream << "CGI reading from client " << clientFd << " pipe " << pipeFd;
	log(stream.str());
	while (true)
	{
		bytesRead = read(pipeStdout, buffer, sizeof(buffer) - 1);
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';
			output += buffer;
			stream.str("");
			stream.clear();
			stream << "CGI read " << output.size() << " bytes...";
			log(stream.str());
		}
		if (bytesRead == 0)
		{
			log("CGI pipe EOF, closing");
			break;
		}
		else if (bytesRead == -1)
			break;
	}
	pipeToRemove = pipeStdout;
	close(pipeStdout);
	waitResult = waitpid(pid, &status, WNOHANG);
	if (waitResult == 0)
	{
		usleep(50000);
		waitpid(pid, &status, WNOHANG);
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		stream.str("");
		stream.clear();
		stream << "CGI script exited with status " << WEXITSTATUS(status);
		logError(stream.str());
	}
	parseCgiOutput(output, response);
	pendingResponses[clientFd] = response.toString();
	log("added CGI output to pending responses");
	i = 0;
	while (i < pollFds.size())
	{
			if (pollFds[i].fd == clientFd)
			{
					pollFds[i].events = POLLOUT;
					break;
			}
			++i;
	}
	stream.str("");
	stream.clear();
	stream << "CGI completed for client " << clientFd << " pipe " << pipeFd;
	log(stream.str());
	clientToPipe.erase(clientFd);
	pipeToServer.erase(pipeStdout);
	cleanup();
	return (pipeToRemove);
}

bool	CgiHandler::isCgiRequest(const RequestContext & context) const
{
	std::vector<std::string>::const_iterator	cgiIt;
	std::string									requestPath;
	std::string									extension;
	size_t										dotPos;

	if (context.getMatchedLocation() == NULL)
		return (false);
	if (context.getMatchedLocation()->getCgiExtensions().empty())
		return (false);
	requestPath = context.getRequestPath();
	dotPos = requestPath.rfind('.');
	if (dotPos == std::string::npos)
		return (false);
	extension = requestPath.substr(dotPos);
	cgiIt = context.getMatchedLocation()->getCgiExtensions().begin();
	while (cgiIt != context.getMatchedLocation()->getCgiExtensions().end())
	{
		if (*cgiIt == extension)
			return (true);
		++cgiIt;
	}
	return (false);
}

bool	CgiHandler::hasActiveCgi() const
{
	return (isActive);
}
