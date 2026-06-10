#include "server/WebServer.hpp"

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

	i = 0;
	while (i < serverConfigs.size())
	{
		Server server(serverConfigs[i]);
		server.setup();
		servers.push_back(server);
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
	std::cout << "[WebServer] Monitoring " << pollFds.size() << " listening sockets" << std::endl;
}

/**
 * There is data to read.
 */
void WebServer::handlePollIn(struct pollfd current)
{
	size_t	i;
	int		clientFd;

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
				std::cout << "[WebServer] New client " << clientFd << " connected" << std::endl;
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
	const char *							data;
	ssize_t									bytesSent;
	size_t									remaining;
	bool									keepAlive;
	bool									removeAfterSend;

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
			removeClient(current.fd);
			pendingResponses.erase(it);
		}
		return;
	}
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
	std::cerr << "[WebServer] Error on fd " << current.fd << std::endl;
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

void WebServer::processClientRequest(int fd)
{
	std::map<int, Client>::iterator	it;
	HttpResponse					response;
	std::string						responseStr;
	std::string						body;

	it = clients.find(fd);
	if (it == clients.end())
		return;
	std::cout << "\n[WebServer] Processing request from fd " << fd << std::endl;
	std::cout << "  Method: " << it->second.getMethod() << std::endl;
	std::cout << "  Path: " << it->second.getPath() << std::endl;
	std::cout << "  Version: " << it->second.getVersion() << std::endl;
	if (it->second.getHeaders().find("Host") != it->second.getHeaders().end())
		std::cout << "  Host: " << it->second.getHeaders().find("Host")->second << std::endl;
	if (it->second.getContentLength() > 0)
		std::cout << "  Content-Length: " << it->second.getContentLength() << std::endl;
	if (!it->second.getBody().empty())
		std::cout << "  Body: " << it->second.getBody() << std::endl;
	body = "<html><body><h1>Hello from webserv!</h1>";
	body += "<p>Received request: " + it->second.getMethod() + " " + it->second.getPath() + "</p>";
	body += "</body></html>";
	response = HttpResponse::ok(body);
	responseStr = response.toString();
	pendingResponses[fd] = responseStr;
	modifyPollEvents(fd, POLLOUT);
}

void WebServer::removeClient(int fd)
{
	std::map<int, std::string>::iterator	pendingIt;
	std::map<int, Client>::iterator			clientIt;

	close(fd);
	clientIt = clients.find(fd);
	if (clientIt != clients.end())
		clients.erase(clientIt);
	pendingIt = pendingResponses.find(fd);
	if (pendingIt != pendingResponses.end())
		pendingResponses.erase(pendingIt);
	clientsToRemove.push_back(fd);
	std::cout << "[WebServer] Client " << fd << " is marked for removal" << std::endl;
}

void WebServer::cleanupRemovedClients()
{
	size_t	clientsCount;
	size_t	pollCount;
	size_t	i;
	size_t	j;

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
	size_t	i;
	int		readyFds;

	getListeningSockets();
	if (pollFds.empty())
	{
		std::cerr << "[WebServer] No listening sockets available. Exiting." << std::endl;
		return;
	}
	running = true;
	std::cout << "[WebServer] Entering event loop..." << std::endl;
	while (running)
	{
		readyFds = poll(&pollFds[0], pollFds.size(), -1);
		if (readyFds == -1)
		{
			if (errno == EINTR)
				continue;
			std::cerr << "Error: poll failed: " << strerror(errno) << std::endl;
			break;
		}
		cleanupRemovedClients();
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
}

void WebServer::stop()
{
	running = false;
}
