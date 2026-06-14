#include "server/WebServer.hpp"
#include "log/log.hpp"
#include "constants.hpp"

WebServer::WebServer()
	: servers(),
	  pollFds(),
	  clients(),
	  clientToServer(),
	  pendingResponses(),
	  clientsToRemove(),
	  pipesToRemove(),
	  running(false),
	  cgiHandler(),
	  clientToPipe(),
	  pipeToServer(),
	  cgiStartTime(),
	  cgiPipeToClient(),
	  sessionManager()
{
}

WebServer::~WebServer()
{
}

WebServer::WebServer(const WebServer & that)
	: servers(that.servers),
	  pollFds(that.pollFds),
	  clients(that.clients),
	  clientToServer(that.clientToServer),
	  pendingResponses(that.pendingResponses),
	  clientsToRemove(that.clientsToRemove),
	  pipesToRemove(that.pipesToRemove),
	  running(that.running),
	  cgiHandler(that.cgiHandler),
	  clientToPipe(that.clientToPipe),
	  pipeToServer(that.pipeToServer),
	  cgiStartTime(that.cgiStartTime),
	  cgiPipeToClient(that.cgiPipeToClient),
	  sessionManager(that.sessionManager)
{
}

WebServer::WebServer(const Config & config)
	: pollFds(),
	  clients(),
	  clientToServer(),
	  pendingResponses(),
	  clientsToRemove(),
	  pipesToRemove(),
	  running(false),
	  cgiHandler(),
	  clientToPipe(),
	  pipeToServer(),
	  cgiStartTime(),
	  cgiPipeToClient(),
	  sessionManager()
{
	const std::vector<ServerConfig> &	serverConfigs = config.getServers();
	size_t								i;
	size_t								j;

	i = 0;
	while (i < serverConfigs.size())
	{
		const std::vector<std::string> & addresses = serverConfigs[i].getListenAddresses();
		j = 0;
		while (j < addresses.size())
		{
			ServerConfig serverConfig = serverConfigs[i];
			std::vector<std::string> address;
			address.push_back(addresses[j]);
			serverConfig.setListenAddresses(address);
			Server server(serverConfig);
			server.setup();
			servers.push_back(server);
			++j;
		}
		++i;
	}
}

WebServer &	WebServer::operator=(const WebServer & that)
{
	if (this != &that)
	{
		servers = that.servers;
		pollFds = that.pollFds;
		clients = that.clients;
		clientToServer = that.clientToServer;
		pendingResponses = that.pendingResponses;
		clientsToRemove = that.clientsToRemove;
		pipesToRemove = that.pipesToRemove;
		running = that.running;
		cgiHandler = that.cgiHandler;
		clientToPipe = that.clientToPipe;
		pipeToServer = that.pipeToServer;
		cgiStartTime = that.cgiStartTime;
		cgiPipeToClient = that.cgiPipeToClient;
		sessionManager = that.sessionManager;
	}
	return (*this);
}

void	WebServer::addToPoll(int fd, short events)
{
	struct pollfd	pollfd;

	pollfd.fd = fd;
	pollfd.events = events;
	pollfd.revents = 0;
	pollFds.push_back(pollfd);
}

void	WebServer::removeFromPoll(int fd)
{
	size_t	i;

	i = 0;
	while (i < pollFds.size())
	{
		if (pollFds[i].fd == fd)
		{
			pollFds.erase(pollFds.begin() + i);
			break;
		}
		++i;
	}
}

void	WebServer::getListeningSockets()
{
	std::ostringstream	stream;
	std::vector<int>	listenFds;
	int					listenFd;
	size_t				i;

	pollFds.clear();
	i = 0;
	while (i < servers.size())
	{
		listenFd = servers[i].getListenFd();
		if (listenFd != -1)
			addToPoll(listenFd, POLLIN);
		++i;
	}
	stream << "webserv is monitoring " << pollFds.size() << " listening sockets";
}

void	WebServer::checkTimeout()
{
	std::map<int, time_t>::iterator		startIt;
	std::map<int, int>::iterator		pipeIt;
	std::ostringstream					stream;
	HttpResponse						response;
	time_t								now;
	time_t								startTime;
	int									pipeFd;
	int									clientFd;

	if (cgiPipeToClient.empty())
		return;
	log("webserv is looking for timed out children");
	now = time(NULL);
	pipeIt = cgiPipeToClient.begin();
	while (pipeIt != cgiPipeToClient.end())
	{
		log("iterating a CGI pipe to client");
		pipeFd = pipeIt->first;
		clientFd = pipeIt->second;
		startIt = cgiStartTime.find(clientFd);
		if (startIt == cgiStartTime.end())
		{
			log("CGI start time not found");
			++pipeIt;
			continue;
		}
		startTime = startIt->second;
		stream << "start time is " << startTime << ", difference is " << (now - startTime) << ", timeout is " << WebServ::CGI_TIMEOUT;
		log(stream.str());
		if ((now - startTime) >= WebServ::CGI_TIMEOUT)
		{
			stream.str("");
			stream.clear();
			stream << "CGI timeout for client " << clientFd
			       << " pipe " << pipeFd
			       << " after " << (now - startTime) << " seconds";
			logError(stream.str());
			cgiHandler.killChild();
			response = HttpResponse::internalServerError(pipeToServer[pipeFd]);
			pendingResponses[clientFd] = response.toString();
			close(pipeFd);
			removeFromPoll(pipeFd);
			pipesToRemove.push_back(pipeFd);
			cgiStartTime.erase(clientFd);
			pipeToServer.erase(pipeFd);
			clientToPipe.erase(clientFd);
			modifyPollEvents(clientFd, POLLOUT);
			pipeIt = cgiPipeToClient.begin();
		}
		else
			++pipeIt;
	}
}

/**
 * There is data to read.
 */
void	WebServer::handlePollIn(struct pollfd current)
{
	std::ostringstream	stream;
	time_t				startTime;
	time_t				now;
	size_t				i;
	bool				timedOut;
	int					clientFd;
	int					pipe;

	stream << "webserv detected a POLLIN event on socket " << current.fd;
	log(stream.str());
	i = 0;
	while (i < servers.size())
	{
		if (servers[i].getListenFd() == current.fd)
		{
			clientFd = servers[i].acceptConnection();
			if (clientFd != -1)
			{
				Client client(clientFd);
				client.setMaxBodySize(servers[i].getConfig().getClientMaxBodySize());
				clients.insert(std::make_pair(clientFd, client));
				addToPoll(clientFd, POLLIN);
				clientToServer[clientFd] = &servers[i];
				stream.str("");
				stream.clear();
				stream << "webserv added a new client socket " << clientFd << " to poll()";
				log(stream.str());
			}
			return;
		}
		++i;
	}
	if (cgiHandler.hasActiveCgi() && pipeToServer.find(current.fd) != pipeToServer.end())
	{
		clientFd = cgiPipeToClient[current.fd];
		startTime = cgiStartTime[clientFd];
		now = time(NULL);
		timedOut = (now - startTime) > WebServ::CGI_TIMEOUT;
		if (timedOut)
		{
			stream.str("");
			stream.clear();
			stream << "CGI timed out for client " << clientFd << " after " << (now - startTime) << " seconds";
			logError(stream.str());
		}
		pipe = cgiHandler.handlePipeOutput(current.fd, timedOut, pendingResponses, pollFds, clientToPipe, pipeToServer);
		if (pipe != -1)
		{
			cgiStartTime.erase(clientFd);
			cgiPipeToClient.erase(pipe);
			stream.str("");
			stream.clear();
			stream << "webserv marked pipe " << pipe << " for removal";
			log(stream.str());
			pipesToRemove.push_back(pipe);
		}
		return;
	}
	handleClientRead(current.fd);
}

/**
 * Writing is now possible, though a write larger than the 
 * available space in a socket or pipe will still block
 * unless O_NONBLOCK is set.
 */
void	WebServer::handlePollOut(struct pollfd current)
{
	std::map<int, std::string>::iterator	it;
	std::map<int, Client>::iterator			clientIt;
	std::ostringstream						stream;
	const char *							data;
	ssize_t									bytesSent;
	size_t									remaining;
	bool									keepAlive;
	bool									removeAfterSend;

	stream << "webserv detected a POLLOUT event on socket " << current.fd;
	log(stream.str());
	it = pendingResponses.find(current.fd);
	if (it == pendingResponses.end())
		return;
	data = it->second.c_str();
	remaining = it->second.size();
	bytesSent = send(current.fd, data, remaining, 0);
	if (bytesSent == -1)
	{
		logError("an error occurred during a call to send()");
		removeClient(current.fd);
		return;
	}
	stream.str("");
	stream.clear();
	stream << "webserv sent " << bytesSent << " bytes";
	log(stream.str());
	if (bytesSent == static_cast<ssize_t>(remaining))
	{
		pendingResponses.erase(it);
		clientIt = clients.find(current.fd);
		if (clientIt != clients.end())
		{
			keepAlive = false; // TODO: implement keep-alive
			removeAfterSend = !keepAlive;
			if (removeAfterSend)
				removeClient(current.fd);
			else
			{
				clientIt->second.resetForNextRequest();
				modifyPollEvents(current.fd, POLLIN);
			}
		}
		else
			removeClient(current.fd);
	}
	else
		it->second = it->second.substr(bytesSent);
}

/**
 * POLLERR: An error condition is triggered. 
 *          This bit is also set for a file descriptor referring to 
 *          the write end of a pipe when the read end has been closed.
 *
 * POLLHUP: The channel is hang up. Note that when reading from a channel 
 *          such as a pipe or a stream socket, this event merely indicates 
 *          that the peer closed its end of the channel.
 *
 * POLLNVAL: Invalid request.
 */
void	WebServer::handlePollErr(struct pollfd current)
{
	std::ostringstream	stream;

	stream << "webserv detected a POLLERR, POLLHUP, or POLLNVAL event on socket " << current.fd;
	logError(stream.str());
	stream.str("");
	stream.clear();
	if (cgiHandler.hasActiveCgi() && pipeToServer.find(current.fd) != pipeToServer.end())
	{
			stream << "handling CGI pipe error/close for fd " << current.fd;
			log(stream.str());
			cgiHandler.handlePipeOutput(current.fd, false, pendingResponses, pollFds, clientToPipe, pipeToServer);
			return;
	}
	close(current.fd);
	removeFromPoll(current.fd);
}

/**
 * Explicitly avoiding determining the server behavior with errno.
 *
 * The purpose of that is to enforce proper non-blocking I/O design 
 * and separation of concerns.
 *
 * Checking errno, after recv() returns -1, makes its behavior 
 * dependent on OS-specific error codes.
 *
 * Different UNIX-like systems may return different error codes 
 * for the same situation.
 *
 * "Checking the value of errno to adjust the server behaviour 
 * is strictly forbidden after performing a read or write operation."
 *
 * If poll() returned POLLIN, recv() should not return -1 unless:
 *
 * - Connection was reset
 * - Or webserv read all data
 */
void	WebServer::handleClientRead(int fd)
{
	std::map<int, Server*>::iterator	serverIt;
	std::map<int, Client>::iterator		clientIt;
	const ServerConfig *				errorConfig;
	std::ostringstream					stream;
	HttpResponse						response;
	ssize_t								bytesRead;
	char								buffer[WebServ::RECV_BUFFER_SIZE];
	bool								keepReading;

	clientIt = clients.find(fd);
	if (clientIt == clients.end())
		return;
	stream << "webserv is reading a client request on socket " << fd;
	log(stream.str());
	keepReading = true;
	while (keepReading)
	{
		bytesRead = recv(fd, buffer, sizeof(buffer), 0);
		if (bytesRead > 0)
		{
			stream.str("");
			stream.clear();
			stream << "webserv read " << bytesRead << " bytes from client socket " << fd;
			log(stream.str());
			clientIt->second.appendToBuffer(std::string(buffer, bytesRead));
			clientIt->second.parseRequest();
			if (clientIt->second.isRequestComplete() || clientIt->second.hasError())
				keepReading = false;
		}
		else if (bytesRead == 0)
		{
			removeClient(fd);
			return;
		}
		else if (bytesRead == -1)
		{
			logError("an error occurred during a call to recv()");
			removeClient(fd);
			keepReading = false;
		}
	}
	if (clientIt->second.hasError())
	{
		stream.str("");
		stream.clear();
		stream << "webserv detected an error with client socket " << fd;
		logError(stream.str());
		serverIt = clientToServer.find(fd);
		if (serverIt != clientToServer.end())
			errorConfig = &(serverIt->second->getConfig());
		else
			errorConfig = NULL;
		if (clientIt->second.isBodySizeExceeded())
			response = HttpResponse::payloadTooLarge(errorConfig);
		else
			response = HttpResponse::badRequest(errorConfig);
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (clientIt->second.isRequestComplete())
		processClientRequest(fd);
}

/**
 * This function determines the appropiate response for a client socket 
 * after its HTTP request has been fully received.
 */
void	WebServer::processClientRequest(int fd)
{
	std::map<int, Client>::iterator	clientIt;
	std::ostringstream				stream;
	RequestContext					context;
	unsigned char					c;
	HttpResponse					response;
	std::string						clientPath;
	std::string						allowedHeader;
	std::string						indexPath;
	std::string						autoindexHTML;
	std::string						requestURI;
	struct stat						statbuf;
	Client *						client;
	size_t							k;
	bool							autoindex;
	bool							hasIndexFile;
	bool							badPath;

	clientIt = clients.find(fd);
	if (clientIt == clients.end())
		return;
	buildRequestContext(fd, context);
	client = &clientIt->second;
	stream << client->getMethod() << " " << client->getPath() << " " << client->getVersion();
	log(stream.str());
	clientPath = client->getPath();
	badPath = clientPath.empty() || clientPath[0] != '/';
	for (k = 0; !badPath && k < clientPath.size(); ++k)
	{
		c = static_cast<unsigned char>(clientPath[k]);
		if (c <= 0x20 || c == 0x7F || c == '<' || c == '>' || c == '"'
			|| c == '{' || c == '}' || c == '|' || c == '\\' || c == '^' || c == '`')
			badPath = true;
	}
	if (badPath)
	{
		logError("400 malformed request target");
		response = HttpResponse::badRequest(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (clientPath.find("/../") != std::string::npos
		|| (clientPath.size() >= 3 && clientPath.compare(clientPath.size() - 3, 3, "/..") == 0))
	{
		logError("403 path traversal attempt");
		response = HttpResponse::forbidden(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (client->getVersion() != WebServ::HTTP_VERSION)
	{
		logError("505 version not supported");
		response = HttpResponse::versionNotSupported(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (client->getMethod() != "GET" && client->getMethod() != "POST" && client->getMethod() != "DELETE")
	{
		logError("501 not implemented");
		response = HttpResponse::notImplemented(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (!isMethodAllowed(context, client->getMethod()))
	{
		logError("405 method not allowed");
		allowedHeader = generateAllowedMethodsHeader(context);
		response = HttpResponse::methodNotAllowed(allowedHeader, context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (client->getPath() == "/session-test" || client->getPath() == "/session-test/")
	{
		handleSessionDemo(fd, *client);
		return;
	}
	if (client->getPath() == "/session-destroy" || client->getPath() == "/session-destroy/")
	{
		handleSessionDestroy(fd, *client);
		return;
	}
	if (client->getPath() == "/api/session" || client->getPath() == "/api/session/")
	{
		handleSessionApi(fd, *client);
		return;
	}
	resolveFilesystemPath(context);
	if (cgiHandler.isCgiRequest(context))
	{
		if (stat(context.getResolvedPath().c_str(), &statbuf) != 0 || !S_ISREG(statbuf.st_mode))
		{
			logError("404 CGI script not found");
			response = HttpResponse::notFound(context.getTargetServer());
			pendingResponses[fd] = response.toString();
			modifyPollEvents(fd, POLLOUT);
			return;
		}
		log("CGI handler is starting");
		cgiStartTime[fd] = time(NULL);
		cgiHandler.startExecution(fd, *client, context, pollFds, clientToPipe, pipeToServer);
		cgiPipeToClient[clientToPipe[fd]] = fd;
		return;
	}
	if (context.hasRedirect())
	{
		handleRedirect(context, response);
		stream.str("");
		stream.clear();
		stream << response.getStatusCode() << " redirected to " << response.getHeaders().find("Location")->second;
		log(stream.str());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	resolveFilesystemPath(context);
	if (client->getMethod() == "POST")
	{
		log("webserv is handling a POST request");
		handlePostRequest(fd, context, *client);
		return;
	}
	if (client->getMethod() == "DELETE")
	{
		log("webserv is handling a DELETE request");
		handleDeleteRequest(fd, context);
		return;
	}
	if (stat(context.getResolvedPath().c_str(), &statbuf) != 0)
	{
		logError(std::string("404 resource ") + context.getResolvedPath() + std::string(" was not found"));
		response = HttpResponse::notFound(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		log(std::string("resource ") + context.getResolvedPath() + std::string(" is a directory"));
		autoindex = false;
		if (context.getMatchedLocation() != NULL)
			autoindex = context.getMatchedLocation()->getAutoindex();
		if (autoindex)
		{
			log("autoindex is enabled");
			indexPath = context.getResolvedPath();
			if (indexPath[indexPath.length() - 1] != '/')
				indexPath += '/';
			if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getIndex().empty())
				indexPath += context.getMatchedLocation()->getIndex();
			else
				indexPath += context.getTargetServer()->getIndex();
			hasIndexFile = (stat(indexPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
			if (!hasIndexFile)
			{
				requestURI = context.getRequestPath();
				if (requestURI.empty() || requestURI[requestURI.length() - 1] != '/')
					requestURI += '/';
				autoindexHTML = generateAutoindex(context.getResolvedPath(), requestURI);
				response.setStatus(200);
				response.setHeader("Content-Type", "text/html");
				response.setBody(autoindexHTML);
				pendingResponses[fd] = response.toString();
				modifyPollEvents(fd, POLLOUT);
				log(std::string("200 generated autoindex for ") + context.getResolvedPath());
				return;
			}
		}
		else
		{
			indexPath = context.getResolvedPath();
			if (indexPath[indexPath.length() - 1] != '/')
				indexPath += '/';
			if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getIndex().empty())
				indexPath += context.getMatchedLocation()->getIndex();
			else
				indexPath += context.getTargetServer()->getIndex();
			hasIndexFile = (stat(indexPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
			if (!hasIndexFile)
			{
				log("index file not found");
				logError("403 autoindex is disabled");
				response = HttpResponse::forbidden(context.getTargetServer());
				pendingResponses[fd] = response.toString();
				modifyPollEvents(fd, POLLOUT);
				return;
			}
		}
		context.setResolvedPath(handleDirectoryPath(context));
		if (stat(context.getResolvedPath().c_str(), &statbuf) != 0)
		{
			logError(std::string("resource ") + context.getResolvedPath() + std::string(" was not found"));
			response = HttpResponse::notFound(context.getTargetServer());
			pendingResponses[fd] = response.toString();
			modifyPollEvents(fd, POLLOUT);
			return;
		}
	}
	if (!S_ISREG(statbuf.st_mode))
	{
		log(std::string("resource ") + context.getResolvedPath() + std::string(" is not a regular file. It may be a device, socket, symlink, or other"));
		response = HttpResponse::forbidden(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	response.setBodyFromFile(context.getResolvedPath());
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("webserv will serve a file located at ") + context.getResolvedPath());
}

void	WebServer::removeClient(int fd)
{
	std::map<int, std::string>::iterator	pendingIt;
	std::map<int, Client>::iterator			clientIt;
	std::map<int, Server*>::iterator		serverIt;
	std::map<int, time_t>::iterator			timeIt;
	std::map<int, int>::iterator			pipeIt;
	std::ostringstream						stream;

	close(fd);
	clientIt = clients.find(fd);
	if (clientIt != clients.end())
		clients.erase(clientIt);
	pendingIt = pendingResponses.find(fd);
	if (pendingIt != pendingResponses.end())
		pendingResponses.erase(pendingIt);
	serverIt = clientToServer.find(fd);
	if (serverIt != clientToServer.end())
		clientToServer.erase(serverIt);
	timeIt = cgiStartTime.find(fd);
	if (timeIt != cgiStartTime.end())
		cgiStartTime.erase(timeIt);
	pipeIt = clientToPipe.find(fd);
	if (pipeIt != clientToPipe.end())
	{
		cgiPipeToClient.erase(pipeIt->second);
		clientToPipe.erase(pipeIt);
	}
	clientsToRemove.push_back(fd);
	stream << "webserv marked client socket " << fd << " for removal";
	log(stream.str());
}

void	WebServer::cleanupRemovedPipes()
{
	std::ostringstream	stream;
	size_t				pipesCount;
	size_t				pollCount;
	size_t				i;
	size_t				j;

	log("webserv is looking for removed pipes");
	pipesCount = pipesToRemove.size();
	pollCount = pollFds.size();
	i = 0;
	while (i < pipesCount)
	{
		j = 0;
		while (j < pollCount)
		{
			if (pollFds[j].fd == pipesToRemove[i])
			{
				stream << "webserv removed pipe " << pollFds[j].fd;
				log(stream.str());
				pollFds.erase(pollFds.begin() + j);
				pollCount = pollCount - 1;
				break;
			}
			++j;
		}
		++i;
	}
	pipesToRemove.clear();
}

void	WebServer::cleanupRemovedClients()
{
	std::ostringstream	stream;
	size_t				clientsCount;
	size_t				pollCount;
	size_t				i;
	size_t				j;

	log("webserv is looking for removed clients");
	clientsCount = clientsToRemove.size();
	pollCount = pollFds.size();
	i = 0;
	while (i < clientsCount)
	{
		j = 0;
		while (j < pollCount)
		{
			if (pollFds[j].fd == clientsToRemove[i])
			{
				stream << "webserv removed client socket " << pollFds[j].fd;
				log(stream.str());
				pollFds.erase(pollFds.begin() + j);
				pollCount = pollCount - 1;
				break;
			}
			++j;
		}
		++i;
	}
	clientsToRemove.clear();
}

void	WebServer::modifyPollEvents(int fd, short events)
{
	size_t	i;

	i = 0;
	while (i < pollFds.size())
	{
		if (pollFds[i].fd == fd)
		{
			pollFds[i].events = events;
			break;
		}
		++i;
	}
}

/**
 * Gathers initial information about the request and stores it in context.
 */
void	WebServer::buildRequestContext(int clientFd, RequestContext & context)
{
	std::map<int, Server*>::iterator	serverIt;
	std::map<int, Client>::iterator		clientIt;
	const LocationConfig *				matchedLocation;
	std::string							requestPath;
	size_t								queryPos;

	clientIt = clients.find(clientFd);
	if (clientIt == clients.end())
		return;
	serverIt = clientToServer.find(clientFd);
	if (serverIt == clientToServer.end())
		return;
	requestPath = clientIt->second.getPath();
	queryPos = requestPath.find('?');
	if (queryPos != std::string::npos)
		requestPath = requestPath.substr(0, queryPos);
	context.setRequestPath(requestPath);
	context.setTargetServer(&(serverIt->second->getConfig()));
	matchedLocation = context.getTargetServer()->matchLocation(requestPath);
	context.setMatchedLocation(matchedLocation);
}

/**
 * Converts the URL path to a filesystem path.
 *
 * Example:
 *
 * If server has a location path "/public" whose root is "./www/public" 
 * and the request is "/public/subdir/file.txt", then:
 *
 * 1: locationPath = "/public"
 * 2: remainingPath = requestPath without locationPath = "/subdir/file.txt"
 * 3: root = "./www/public"
 * 4: resolved = root + remainingPath = "./www/public/subdir/file.txt"
 */
void	WebServer::resolveFilesystemPath(RequestContext & context)
{
	std::string	remainingPath;
	std::string	locationPath;
	std::string	requestPath;
	std::string	resolved;
	std::string	root;

	requestPath = context.getRequestPath();
	root = context.getTargetServer()->getRoot();
	if (context.hasCustomRoot())
		root = context.getMatchedLocation()->getRoot();
	if (context.getMatchedLocation() != NULL)
	{
		locationPath = context.getMatchedLocation()->getPath();
		if (requestPath.find(locationPath) == 0)
		{
			remainingPath = requestPath.substr(locationPath.length());
			if (remainingPath.empty() || remainingPath[0] != '/')
				remainingPath = "/" + remainingPath;
		}
		else
			remainingPath = requestPath;
	}
	else
		remainingPath = requestPath;
	if (root[root.length() - 1] == '/')
		resolved = root;
	else
		resolved = root + "/";
	if (!remainingPath.empty() && remainingPath[0] == '/')
		remainingPath = remainingPath.substr(1);
	resolved += remainingPath;
	context.setResolvedPath(resolved);
}

/**
 * If the location has a return directive, creates a redirect response.
 *
 * Example:
 *
 * For a given location block:
 *
 * location /old-stuff {
 *     return 301 /public;
 * }
 *
 * 1. Detects hasRedirect() = true
 * 2. Reads redirect target = /public
 * 3. Detects 301 in the string
 * 4. Creates HTTP response: 301 Moved Permanently with Location: /public
 */
void	WebServer::handleRedirect(const RequestContext & context, HttpResponse & response)
{
	std::string	redirectCode;
	std::string	redirectPath;
	int			statusCode;

	if (!context.hasRedirect())
		return;
	redirectCode = context.getMatchedLocation()->getRedirectCode();
	redirectPath = context.getMatchedLocation()->getRedirect();
	if (redirectCode == "302")
		statusCode = 302;
	else
		statusCode = 301;
	if (statusCode == 302)
		response = HttpResponse::found(redirectPath);
	else
		response = HttpResponse::movedPermanently(redirectPath);
}

/**
 * Checks if the HTTP method is allowed for this location.
 *
 * If the context lacks a matched location, all methods are allowed.
 *
 * This is intentional, because to forbid methods, the configuration 
 * file allows the administrator to explicitly forbid all methods 
 * for a location if "allow_methods" directive is missing.
 *
 * Example:
 *
 * Given the following location blocks:
 *
 * location /public {
 *     allow_methods GET;
 * }
 * 
 * location /api {
 *     allow_methods GET POST DELETE;
 * }
 * 
 * location / {
 * }
 *
 * A request whose method is POST is allowed for path "/api" 
 * but not allowed for path "/public".
 *
 * All methods are forbidden for path "/".
 */
bool	WebServer::isMethodAllowed(const RequestContext & context, const std::string & method)
{
	const std::vector<std::string> *	allowedMethods;
	size_t								i;

	if (context.getMatchedLocation() == NULL)
		return (true);
	allowedMethods = &(context.getMatchedLocation()->getAllowedMethods());
	if (allowedMethods->empty())
		return (false);
	i = 0;
	while (i < allowedMethods->size())
	{
		if ((*allowedMethods)[i] == method)
			return (true);
		++i;
	}
	return (false);
}

bool	WebServer::isDirectory(const std::string & path)
{
	struct stat	statbuf;
	int			result;

	result = stat(path.c_str(), &statbuf);
	if (result != 0)
		return (false);
	return (S_ISDIR(statbuf.st_mode));
}

/**
 * When the resolved path is a directory, determines what file to serve.
 *
 * Priority chain:
 *
 * 1. Location's index directive (if specified)
 * 2. Server's index directive (if specified)
 * 3. Default "index.html"
 */
std::string	WebServer::handleDirectoryPath(RequestContext & context)
{
	std::string	resolvedPath;
	std::string	indexFile;
	std::string	indexPath;

	resolvedPath = context.getResolvedPath();
	if (resolvedPath[resolvedPath.length() - 1] != '/')
		resolvedPath += '/';
	if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getIndex().empty())
		indexFile = context.getMatchedLocation()->getIndex();
	else
		indexFile = context.getTargetServer()->getIndex();
	if (indexFile.empty())
		indexFile = WebServ::DEFAULT_INDEX;
	indexPath = resolvedPath + indexFile;
	return (indexPath);
}

std::string	WebServer::generateAutoindex(const std::string & dirPath, const std::string & requestPath)
{
	std::vector<std::string>	items;
	struct dirent *				entry;
	std::string					requestPathWithSlash;
	std::string					displayPath;
	std::string					fullPath;
	std::string					result;
	struct stat					entryStat;
	size_t						i;
	DIR *						dir;
	char						buffer[64];

	dir = opendir(dirPath.c_str());
	if (dir == NULL)
		return ("");
	result = "<html>\n<head>\n<title>Index of ";
	result += escapeHtml(requestPath);
	result += "</title>\n</head>\n<body>\n<h1>Index of ";
	result += escapeHtml(requestPath);
	result += "</h1>\n<hr>\n<pre>\n";
	requestPathWithSlash = requestPath;
	if (requestPathWithSlash.empty() || requestPathWithSlash[requestPathWithSlash.length() - 1] != '/')
		requestPathWithSlash += '/';
	if (requestPath != "/")
		result += "<a href=\"../\">../</a>\n";
	while (true)
	{
		entry = readdir(dir);
		if (entry == NULL)
			break;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '\0')
			continue;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0')
			continue;
		items.push_back(std::string(entry->d_name));
	}
	closedir(dir);
	i = 0;
	while (i < items.size())
	{
		fullPath = dirPath;
		if (fullPath[fullPath.length() - 1] != '/')
			fullPath += '/';
		fullPath += items[i];
		if (stat(fullPath.c_str(), &entryStat) == 0)
		{
			result += "<a href=\"";
			result += escapeHtml(items[i]);
			if (S_ISDIR(entryStat.st_mode))
				result += "/";
			result += "\">";
			result += escapeHtml(items[i]);
			if (S_ISDIR(entryStat.st_mode))
				result += "/";
			result += "</a>";
			if (S_ISDIR(entryStat.st_mode))
				result += "                    -";
			else
			{
				while (result.length() < 50)
					result += " ";
				result += formatFileSize(entryStat.st_size);
			}
			while (result.length() < 70)
				result += " ";
			strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", localtime(&entryStat.st_mtime));
			result += buffer;
			result += "\n";
		}
		++i;
	}
	result += "</pre>\n<hr>\n</body>\n</html>\n";
	return (result);
}

std::string	WebServer::formatFileSize(off_t size)
{
	std::ostringstream	stream;
	char				buffer[64];

	if (size < 1024)
		stream << size << " B";
	else if (size < 1024 * 1024)
	{
		std::snprintf(buffer, sizeof(buffer), "%.1f KB", size / 1024.0);
		stream << buffer;
	}
	else if (size < 1024 * 1024 * 1024)
	{
		std::snprintf(buffer, sizeof(buffer), "%.1f MB", size / (1024.0 * 1024.0));
		stream << buffer;
	}
	else
	{
		std::snprintf(buffer, sizeof(buffer), "%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
		stream << buffer;
	}
	return (stream.str());
}

std::string	WebServer::escapeHtml(const std::string & str)
{
	std::string	result;
	size_t		i;
	char		c;

	i = 0;
	while (i < str.length())
	{
		c = str[i];
		if (c == '&')
			result += "&amp;";
		else if (c == '<')
			result += "&lt;";
		else if (c == '>')
			result += "&gt;";
		else if (c == '"')
			result += "&quot;";
		else
			result += c;
		++i;
	}
	return (result);
}

bool	WebServer::validateBodySize(const Client & client, const ServerConfig & config, HttpResponse & response)
{
	size_t	contentLength;

	contentLength = client.getContentLength();
	if (contentLength > config.getClientMaxBodySize())
	{
		response = HttpResponse::payloadTooLarge(&config);
		return (false);
	}
	return (true);
}

std::string	WebServer::getUploadPath(const RequestContext & context)
{
	std::ostringstream	stream;
	std::string			uploadPath;
	std::string			filename;
	size_t				lastSlash;

	if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getUploadStore().empty())
		uploadPath = context.getMatchedLocation()->getUploadStore();
	else
		uploadPath = WebServ::DEFAULT_UPLOADS;
	if (uploadPath[uploadPath.length() - 1] != '/')
		uploadPath += '/';
	filename = context.getRequestPath();
	lastSlash = filename.rfind('/');
	if (lastSlash != std::string::npos)
		filename = filename.substr(lastSlash + 1);
	if (filename.empty())
	{
		stream << time(NULL);
		filename = stream.str();
	}
	uploadPath += filename;
	return (uploadPath);
}

std::string	WebServer::generateAllowedMethodsHeader(const RequestContext & context)
{
	const std::vector<std::string> *	allowedMethods;
	std::string							result;
	size_t								i;

	if (context.getMatchedLocation() == NULL)
		return ("GET, POST, DELETE");
	allowedMethods = &(context.getMatchedLocation()->getAllowedMethods());
	i = 0;
	while (i < allowedMethods->size())
	{
		if (i > 0)
			result += ", ";
		result += (*allowedMethods)[i];
		++i;
	}
	return (result);
}

/**
 * The flag std::ios::out opens a file for writing.
 *
 * The flag std::ios::binary opens a file in binary mode:
 *
 * When a file is opened in C++, there are two modes 
 * for handling newline characters:
 *
 * - text mode (default): On Windows, \n gets converted to \r\n on write. 
 *                        On Unix-like systems (Linux, macOS), 
 *                        no conversion happens.
 *
 * - binary mode: No newline conversion. Bytes are written as given.
 */
void	WebServer::handlePostRequest(int fd, RequestContext & context, Client & client)
{
	std::ofstream	file;
	HttpResponse	response;
	std::string		uploadPath;

	if (!validateBodySize(client, *(context.getTargetServer()), response))
	{
		logError("413 payload too large");
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (context.getMatchedLocation() == NULL || context.getMatchedLocation()->getUploadStore().empty())
	{
		logError("501 not implemented");
		response = HttpResponse::notImplemented(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	uploadPath = getUploadPath(context);
	file.open(uploadPath.c_str(), std::ios::out | std::ios::binary);
	if (!file.is_open())
	{
		logError("500 internal server error");
		response = HttpResponse::internalServerError(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	file.write(client.getUnchunkedBody().c_str(), client.getUnchunkedBody().size());
	file.close();
	response = HttpResponse::created(uploadPath);
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("uploaded file to ") + uploadPath);
}

void	WebServer::handleDeleteRequest(int fd, RequestContext & context)
{
	HttpResponse	response;
	std::string		targetPath;
	struct stat		statbuf;

	if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getUploadStore().empty())
		targetPath = getUploadPath(context);
	else
		targetPath = context.getResolvedPath();
	if (stat(targetPath.c_str(), &statbuf) != 0)
	{
		logError("404 not found");
		response = HttpResponse::notFound(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		logError("403 forbidden");
		response = HttpResponse::forbidden(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		response = HttpResponse::forbidden(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (access(targetPath.c_str(), W_OK) != 0)
	{
		logError("403 forbidden");
		response = HttpResponse::forbidden(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (unlink(targetPath.c_str()) != 0)
	{
		logError("500 internal server error");
		response = HttpResponse::internalServerError(context.getTargetServer());
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	response = HttpResponse::noContent();
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("deleted file ") + targetPath);
}

void	WebServer::sendSessionResponse(int fd, const std::string & html, const std::string & cookieHeader)
{
	HttpResponse	response;

	response.setStatus(200);
	response.setHeader("Content-Type", "text/html");
	if (!cookieHeader.empty())
		response.setHeader("Set-Cookie", cookieHeader);
	response.setBody(html);
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
}

void	WebServer::handleSessionDemo(int fd, Client & client)
{
	std::ostringstream	counter;
	std::string			setCookieHeader;
	std::string			cookieHeader;
	std::string			visitCount;
	std::string			sessionId;
	std::string			html;
	Session *			session;

	cookieHeader = "";
	if (client.getHeaders().find("Cookie") != client.getHeaders().end())
		cookieHeader = client.getHeaders().find("Cookie")->second;
	sessionId = sessionManager.extractSessionIdFromCookie(cookieHeader);
	if (sessionId.empty())
	{
		sessionId = sessionManager.createSession();
		session = sessionManager.getSession(sessionId);
		sessionManager.addSessionData(sessionId, "visits", "1");
		setCookieHeader = "session_id=" + sessionId + "; Path=/; HttpOnly";
		visitCount = "1";
		log(std::string("new session created: ") + sessionId);
	}
	else
	{
		session = sessionManager.getSession(sessionId);
		if (session == NULL)
		{
			sessionId = sessionManager.createSession();
			session = sessionManager.getSession(sessionId);
			sessionManager.addSessionData(sessionId, "visits", "1");
			setCookieHeader = "session_id=" + sessionId + "; Path=/; HttpOnly";
			visitCount = "1";
			log(std::string("expired session replaced: ") + sessionId);
		}
		else
		{
			visitCount = sessionManager.getSessionData(sessionId, "visits");
			if (visitCount.empty())
				visitCount = "0";
			counter << (std::atoi(visitCount.c_str()) + 1);
			sessionManager.addSessionData(sessionId, "visits", counter.str());
			visitCount = counter.str();
			log(std::string("session ") + sessionId + " visit count: " + visitCount);
		}
	}
	html = "<!DOCTYPE html>\n";
	html += "<html>\n<head>\n<title>Session Demo</title>\n</head>\n<body>\n";
	html += "<h1>Session Demo</h1>\n";
	html += "<p><strong>Session ID:</strong> " + sessionId + "</p>\n";
	html += "<p><strong>Visit Count:</strong> " + visitCount + "</p>\n";
	html += "<hr>\n";
	html += "<a href='/session-test'>Refresh (increment visit count)</a><br>\n";
	html += "<a href='/session-destroy'>Destroy Session</a><br>\n";
	html += "<a href='/session/demo.html'>Go to Static Session Page</a>\n";
	html += "</body>\n</html>\n";
	sendSessionResponse(fd, html, setCookieHeader);
}

void	WebServer::handleSessionDestroy(int fd, Client & client)
{
	std::string	cookieHeader;
	std::string	sessionId;
	std::string	html;

	cookieHeader = "";
	if (client.getHeaders().find("Cookie") != client.getHeaders().end())
		cookieHeader = client.getHeaders().find("Cookie")->second;
	sessionId = sessionManager.extractSessionIdFromCookie(cookieHeader);
	if (!sessionId.empty())
		sessionManager.destroySession(sessionId);
	html = "<!DOCTYPE html>\n";
	html += "<html>\n<head>\n<title>Session Destroyed</title>\n</head>\n<body>\n";
	html += "<h1>Session Destroyed</h1>\n";
	html += "<p>Your session has been destroyed.</p>\n";
	html += "<a href='/session-test'>Create New Session</a>\n";
	html += "</body>\n</html>\n";
	sendSessionResponse(fd, html, "session_id=; Path=/; Max-Age=0");
}

void	WebServer::handleSessionApi(int fd, Client & client)
{
	HttpResponse	response;
	std::string		cookieHeader;
	std::string		sessionId;
	std::string		responseBody;
	std::string		setCookieHeader;
	std::string		visits;
	std::string		stored;
	Session *		session;

	cookieHeader = "";
	if (client.getHeaders().find("Cookie") != client.getHeaders().end())
		cookieHeader = client.getHeaders().find("Cookie")->second;
	sessionId = sessionManager.extractSessionIdFromCookie(cookieHeader);
	if (client.getMethod() == "DELETE")
	{
		if (!sessionId.empty())
			sessionManager.destroySession(sessionId);
		response.setStatus(200);
		response.setHeader("Set-Cookie", "session_id=; Path=/; Max-Age=0");
		response.setHeader("Content-Type", "application/json");
		response.setBody("{\"status\": \"session destroyed\"}");
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (sessionId.empty())
	{
		sessionId = sessionManager.createSession();
		session = sessionManager.getSession(sessionId);
		sessionManager.addSessionData(sessionId, "visits", "1");
		setCookieHeader = "session_id=" + sessionId + "; Path=/; HttpOnly";
	}
	else
	{
		session = sessionManager.getSession(sessionId);
		if (session == NULL)
		{
			sessionId = sessionManager.createSession();
			session = sessionManager.getSession(sessionId);
			sessionManager.addSessionData(sessionId, "visits", "1");
			setCookieHeader = "session_id=" + sessionId + "; Path=/; HttpOnly";
		}
		else
			setCookieHeader = "";
	}
	response.setStatus(200);
	if (!setCookieHeader.empty())
		response.setHeader("Set-Cookie", setCookieHeader);
	response.setHeader("Content-Type", "application/json");
	if (client.getMethod() == "POST")
	{
		sessionManager.addSessionData(sessionId, "stored_data", client.getBody());
		responseBody = "{\"status\": \"data stored\", \"session_id\": \"" + sessionId + "\"}";
	}
	else
	{
		visits = sessionManager.getSessionData(sessionId, "visits");
		stored = sessionManager.getSessionData(sessionId, "stored_data");
		responseBody = "{\"session_id\": \"" + sessionId + "\", \"visits\": " + visits + ", \"stored_data\": \"" + stored + "\"}";
	}
	response.setBody(responseBody);
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
}

std::string	WebServer::getSetCookieHeader(Session * session)
{
	std::ostringstream	stream;

	if (session == NULL)
		return ("");
	stream << "session_id=" << session->id << "; Path=/; HttpOnly";
	return (stream.str());
}

void	WebServer::handleSession(Client & client, HttpResponse & response)
{
	std::map<std::string, std::string>::const_iterator	it;
	std::ostringstream									counter;
	std::string											cookieHeader;
	std::string											sessionId;
	std::string											visitCount;
	Session *											session;

	it = client.getHeaders().find("Cookie");
	if (it != client.getHeaders().end())
		cookieHeader = it->second;
	if (!cookieHeader.empty())
		sessionId = sessionManager.extractSessionIdFromCookie(cookieHeader);
	if (sessionId.empty())
	{
		sessionId = sessionManager.createSession();
		session = sessionManager.getSession(sessionId);
		sessionManager.addSessionData(sessionId, "visits", "1");
		response.setHeader("Set-Cookie", getSetCookieHeader(session));
		log(std::string("new session created: ") + sessionId);
	}
	else
	{
		session = sessionManager.getSession(sessionId);
		if (session == NULL)
		{
			sessionId = sessionManager.createSession();
			session = sessionManager.getSession(sessionId);
			sessionManager.addSessionData(sessionId, "visits", "1");
			response.setHeader("Set-Cookie", getSetCookieHeader(session));
			log(std::string("expired session, new session created: ") + sessionId);
		}
		else
		{
			visitCount = sessionManager.getSessionData(sessionId, "visits");
			if (visitCount.empty())
				visitCount = "0";
			counter << (std::atoi(visitCount.c_str()) + 1);
			sessionManager.addSessionData(sessionId, "visits", counter.str());
			log(std::string("session ") + sessionId + " visit count: " + counter.str());
		}
	}
}

/**
 * A socket that has both data to read AND can write 
 * revents could be: POLLIN | POLLOUT (both bits set)
 *
 * If the control structure is written as:
 * 
 * if (POLLIN) {}
 * else if (POLLOUT) {}
 *
 * It's wrong because the server will handle POLLIN only
 */
void	WebServer::run()
{
	extern volatile	sig_atomic_t	g_running;
	size_t							i;
	int								readyFds;

	getListeningSockets();
	if (pollFds.empty())
	{
		logError("webserv lacks listening sockets, exiting...");
		return;
	}
	running = true;
	log("webserv is entering to the event loop");
	while (running && g_running)
	{
		cleanupRemovedClients();
		cleanupRemovedPipes();
		checkTimeout();
		log("webserv is executing poll()");
		readyFds = poll(&pollFds[0], pollFds.size(), WebServ::CGI_TIMEOUT * 1000);
		if (readyFds == -1)
		{
			if (errno == EINTR && !g_running)
				break;
			logError(std::string("poll() failed: ") + strerror(errno));
			break;
		}
		i = 0;
		while (i < pollFds.size())
		{
			if (pollFds[i].revents & POLLIN)
				handlePollIn(pollFds[i]);
			if (pollFds[i].revents & POLLOUT)
				handlePollOut(pollFds[i]);
			if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				handlePollErr(pollFds[i]);
				continue;
			}
			++i;
		}
	}
	log("webserv is shutting down");
	i = 0;
	while (i < pollFds.size())
	{
		close(pollFds[i].fd);
		++i;
	}
	pollFds.clear();
	clients.clear();
	clientToServer.clear();
	pendingResponses.clear();
}

void	WebServer::stop()
{
	running = false;
}
