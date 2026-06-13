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
	contentLengthStream << client.getBody().size();
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

std::string	CgiHandler::parseCgiOutput(const std::string & output) const
{
	std::string	body;
	size_t		headerEnd;

	headerEnd = output.find(WebServ::CRLF + WebServ::CRLF);
	if (headerEnd != std::string::npos)
		body = output.substr(headerEnd + 4);
	else
		body = output;
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
	stream << "executing CGI script " << scriptPath << " with " << interpreter;
	log(stream.str());
	if (pipe(pipeStdin) == -1 || pipe(pipeStdout) == -1)
	{
		logError(std::string("pipe() failed"));
		return;
	}
	pid = fork();
	if (pid == -1)
	{
		logError(std::string("fork() failed"));
		close(pipeStdin[0]);
		close(pipeStdin[1]);
		close(pipeStdout[0]);
		close(pipeStdout[1]);
		return;
	}
	if (pid == 0)
	{
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
	close(pipeStdin[0]);
	close(pipeStdout[1]);
	stdinWriteFd = pipeStdin[1];
	stdoutReadFd = pipeStdout[0];
	write(stdinWriteFd, client.getBody().c_str(), client.getBody().size());
	close(stdinWriteFd);
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
	stream << "CGI started for client " << clientFd << ", monitoring pipe " << stdoutReadFd;
	log(stream.str());
}

void	CgiHandler::handlePipeOutput(int pipeFd, std::map<int, std::string> & pendingResponses, std::vector<struct pollfd> & pollFds, std::map<int, int> & clientToPipe, std::map<int, const ServerConfig *> & pipeToServer)
{
	std::ostringstream	stream;
	HttpResponse		response;
	ssize_t				bytesRead;
	size_t				i;
	pid_t				waitResult;
	char				buffer[WebServ::CGI_BUFFER];
	int					status;
	int					aux;

	if (!isActive || pipeStdout != pipeFd)
		return;
	stream << "CGI reading from pipe " << pipeFd;
	log(stream.str());
	bytesRead = read(pipeStdout, buffer, sizeof(buffer) - 1);
	stream.str("");
	stream.clear();
	stream << "CGI read returned " << bytesRead << " bytes";
	log(stream.str());
	stream.str("");
	stream.clear();
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		output += buffer;
		stream << "CGI accumulated output: " << output.size() << " bytes";
		log(stream.str());
		return;
	}
	if (bytesRead == 0)
		log("CGI pipe EOF, closing");
	else
		logError("CGI pipe error");
	close(pipeStdout);
	stream.str("");
	stream.clear();
	i = 0;
	while (i < pollFds.size())
	{
		if (pollFds[i].fd == pipeStdout)
		{
			pollFds.erase(pollFds.begin() + i);
			stream << "Removed pipe " << pipeStdout << " from pollFds";
			log(stream.str());
			break;
		}
		++i;
	}
	waitResult = waitpid(pid, &status, WNOHANG);
	if (waitResult == 0)
	{
		usleep(50000);
		waitpid(pid, &status, WNOHANG);
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		stream << "CGI script exited with status " << WEXITSTATUS(status);
		logError(stream.str());
	}
	response.setStatus(200);
	response.setHeader("Content-Type", "text/html");
	response.setBody(parseCgiOutput(output));
	pendingResponses[clientFd] = response.toString();
	stream.str("");
	stream.clear();
	i = 0;
	while (i < pollFds.size())
	{
			if (pollFds[i].fd == clientFd)
			{
					pollFds[i].events = POLLOUT;
					stream << "Changed client " << clientFd << " poll events to POLLOUT";
					log(stream.str());
					break;
			}
			++i;
	}
	aux = clientFd;
	clientToPipe.erase(clientFd);
	pipeToServer.erase(pipeStdout);
	cleanup();
	stream.str("");
	stream.clear();
	stream << "CGI completed for client " << aux;
	log(stream.str());
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
