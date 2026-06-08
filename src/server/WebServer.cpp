#include "server/WebServer.hpp"

WebServer::WebServer(Config const & config) : pollFds(), running(false)
{
	std::vector<ServerConfig> const &	serverConfigs = config.getServers();
	size_t								i;

	i = 0;
	while (i < serverConfigs.size())
	{
		Server *server = new Server(serverConfigs[i]);
		server->setup();
		servers.push_back(server);
		++i;
	}
}

WebServer::~WebServer()
{
	size_t	i;

	i = 0;
	while (i < servers.size())
	{
		delete servers[i];
		++i;
	}
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
		listenFd = servers[i]->getListenFd();
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
	struct pollfd	client;
	size_t			i;
	int				clientFd;

	i = 0;
	while (i < servers.size())
	{
		if (servers[i]->getListenFd() == current.fd)
		{
			clientFd = servers[i]->acceptConnection();
			if (clientFd != -1)
			{
				client.fd = clientFd;
				client.events = POLLIN;
				client.revents = 0;
				pollFds.push_back(client);
			}
			break;
		}
		++i;
	}
}

/**
 * Writing is now possible, though a write larger than the 
 * available space in a socket or pipe will still block
 * unless O_NONBLOCK is set.
 */
void WebServer::handlePollOut(struct pollfd current)
{
	std::cout << "[Server] Ready to write on fd " << current.fd << std::endl;
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
