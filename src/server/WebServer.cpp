#include "server/WebServer.hpp"
#include "log/log.hpp"

WebServer::WebServer()
	: servers(),
	  pollFds(),
	  clients(),
	  pendingResponses(),
	  running(false)
{
}

WebServer::~WebServer()
{
}

WebServer::WebServer(const WebServer & that)
	: servers(that.servers),
	  pollFds(that.pollFds),
	  clients(that.clients),
	  pendingResponses(that.pendingResponses),
	  running(that.running)
{
}

WebServer::WebServer(const Config & config)
	: pollFds(),
	  clients(),
	  pendingResponses(),
	  running(false)
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
		pendingResponses = that.pendingResponses;
		running = that.running;
	}
	return (*this);
}

void WebServer::addToPoll(int fd, short events)
{
	struct pollfd	pollfd;

	pollfd.fd = fd;
	pollfd.events = events;
	pollfd.revents = 0;
	pollFds.push_back(pollfd);
}

void WebServer::removeFromPoll(int fd)
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

void WebServer::getListeningSockets()
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

/**
 * There is data to read.
 */
void WebServer::handlePollIn(struct pollfd current)
{
	std::ostringstream	stream;
	size_t				i;
	int					clientFd;

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
				clients.insert(std::make_pair(clientFd, Client(clientFd)));
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
	handleClientRead(current.fd);
}

/**
 * Writing is now possible, though a write larger than the 
 * available space in a socket or pipe will still block
 * unless O_NONBLOCK is set.
 */
void WebServer::handlePollOut(struct pollfd current)
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
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			logError(std::string("send() failed: ") + strerror(errno));
			removeClient(current.fd);
			pendingResponses.erase(it);
		}
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
void WebServer::handlePollErr(struct pollfd current)
{
	std::ostringstream	stream;

	stream << "webserv detected a POLLERR, POLLHUP, or POLLNVAL event on socket " << current.fd;
	logError(stream.str());
	close(current.fd);
	removeFromPoll(current.fd);
}

void WebServer::handleClientRead(int fd)
{
	std::map<int, Client>::iterator	it;
	char							buffer[WebServ::RECV_BUFFER_SIZE];
	ssize_t							bytesRead;
	bool							keepReading;

	it = clients.find(fd);
	if (it == clients.end())
		return;
	keepReading = true;
	while (keepReading)
	{
		bytesRead = recv(fd, buffer, sizeof(buffer), 0);
		if (bytesRead > 0)
		{
			it->second.appendToBuffer(std::string(buffer, bytesRead));
			it->second.parseRequest();
			if (it->second.isRequestComplete() || it->second.hasError())
				keepReading = false;
		}
		else if (bytesRead == 0)
		{
			removeClient(fd);
			return;
		}
		else
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				keepReading = false;
			else
				removeClient(fd);
			return;
		}
	}
	if (it->second.isRequestComplete())
		processClientRequest(fd);
	else if (it->second.hasError())
		removeClient(fd);
}

/**
 * Client Request (GET /index.html)
 *          │
 *          ▼
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ 1. Find Client object by FD                                     │
 * │ 2. Find which Server owns this client                           │
 * │ 3. Check for parsing errors (400 Bad Request)                   │
 * │ 4. Convert request path to filesystem path                      │
 * │ 5. Check if file exists using stat()                            │
 * │ 6. If directory → append index.html and check again             │
 * │ 7. If not a regular file → 403 Forbidden                        │
 * │      Ensures the path is a regular file,                        │
 * │      not a special file (device, pipe, socket, symlink)         │
 * │ 8. Read file and send as response                               │
 * └─────────────────────────────────────────────────────────────────┘
 */
void WebServer::processClientRequest(int fd)
{
	std::map<int, Client>::iterator		clientIt;
	std::map<int, Server*>::iterator	serverIt;
	HttpResponse						response;
	std::string							fullPath;
	struct stat							statbuf;

	clientIt = clients.find(fd);
	if (clientIt == clients.end())
		return;
	serverIt = clientToServer.find(fd);
	if (serverIt == clientToServer.end())
	{
		response = HttpResponse::internalServerError();
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (clientIt->second.hasError())
	{
		response = HttpResponse::badRequest();
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	fullPath = buildFilePath(clientIt->second, serverIt->second->getConfig());
	if (stat(fullPath.c_str(), &statbuf) != 0)
	{
		response = HttpResponse::notFound();
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		fullPath = handleDirectoryPath(fullPath, serverIt->second->getConfig());
		if (stat(fullPath.c_str(), &statbuf) != 0)
		{
			response = HttpResponse::notFound();
			pendingResponses[fd] = response.toString();
			modifyPollEvents(fd, POLLOUT);
			return;
		}
	}
	if (!S_ISREG(statbuf.st_mode))
	{
		response = HttpResponse::forbidden();
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	response.setBodyFromFile(fullPath);
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("webserv is serving a file located at ") + fullPath);
}

void WebServer::removeClient(int fd)
{
	std::map<int, std::string>::iterator	pendingIt;
	std::map<int, Client>::iterator			clientIt;
	std::map<int, Server*>::iterator		serverIt;
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
	clientsToRemove.push_back(fd);
	stream << "webserv marked client socket " << fd << " for removal";
	log(stream.str());
}

void WebServer::cleanupRemovedClients()
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

void WebServer::modifyPollEvents(int fd, short events)
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

std::string WebServer::buildFilePath(const Client & client, const ServerConfig & serverConfig)
{
	std::string	requestPath;
	std::string	root;
	std::string	fullPath;
	size_t		queryPos;

	requestPath = client.getPath();
	root = serverConfig.getRoot();
	queryPos = requestPath.find('?');
	if (queryPos != std::string::npos)
		requestPath = requestPath.substr(0, queryPos);
	if (requestPath.length() > 1 && requestPath[requestPath.length() - 1] == '/')
		requestPath = requestPath.substr(0, requestPath.length() - 1);
	fullPath = root;
	if (fullPath[fullPath.length() - 1] != '/')
		fullPath += '/';
	if (!requestPath.empty() && requestPath[0] == '/')
		requestPath = requestPath.substr(1);
	fullPath += requestPath;
	return (fullPath);
}

bool WebServer::isDirectory(const std::string & path)
{
	struct stat	statbuf;
	int			result;

	result = stat(path.c_str(), &statbuf);
	if (result != 0)
		return (false);
	return (S_ISDIR(statbuf.st_mode));
}

std::string WebServer::handleDirectoryPath(const std::string & dirPath, const ServerConfig & serverConfig)
{
	std::string	indexPath;

	indexPath = dirPath;
	if (indexPath[indexPath.length() - 1] != '/')
		indexPath += '/';
	indexPath += serverConfig.getIndex();
	return (indexPath);
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
void WebServer::run()
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
		log("webserv is executing poll()");
		readyFds = poll(&pollFds[0], pollFds.size(), -1);
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

void WebServer::stop()
{
	running = false;
}
