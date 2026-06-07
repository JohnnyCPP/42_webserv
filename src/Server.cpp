#include "Server.hpp"
#include "constants.hpp"

Server::Server(ServerConfig const & config) : config(config), listenFd(-1), pollFds(), running(false), host(""), port(0)
{
}

Server::Server(Server const & that) : config(that.config), listenFd(-1), pollFds(that.pollFds), running(false), host(that.host), port(0)
{
}

Server::~Server()
{
	size_t	i;

	if (listenFd != -1)
		close(listenFd);
	i = 0;
	while (i < pollFds.size())
	{
		if (pollFds[i].fd != -1)
			close(pollFds[i].fd);
		++i;
	}
}

Server & Server::operator=(Server const & that)
{
	if (this != &that)
	{
		config = that.config;
		listenFd = -1;
		pollFds = that.pollFds;
		running = false;
		host = that.host;
		port = 0;
	}
	return (*this);
}

/**
 * The core difference of O_NONBLOCK:
 *
 * - Blocking socket
 *     When any of read()/recv() are called and no data is available, 
 *     the function stops the program completely until data arrives.
 *
 * - Non-blocking socket
 *     When any of read()/recv() are called and no data is available, 
 *     the function returns immediately with an error 
 *     (errno = EAGAIN/EWOULDBLOCK).
 *
 * Both errors EAGAIN/EWOULDBLOCK mean: 
 * "The operation would block, but the file descriptor is marked non-blocking"
 *
 * EAGAIN = "Try again" - Resource temporarily unavailable
 * EWOULDBLOCK = "Would block" - Operation would block
 *
 * For historical reasons, both exist and code should 
 * check both to be portable.
 *
 * They can be checked with "errno" on recv() calls.
 */
void Server::makeNonBlocking(int fd)
{
	int	flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::cerr << "Error: fcntl F_GETFL failed" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		std::cerr << "Error: fcntl F_SETFL O_NONBLOCK failed" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void Server::parseListenAddress(std::string const & addr, std::string & host, int & port)
{
	size_t	colonPos;
	bool	isAllDigits;
	size_t	i;

	colonPos = addr.find(':');
	if (colonPos != std::string::npos)
	{
		host = addr.substr(0, colonPos);
		std::istringstream port_stream(addr.substr(colonPos + 1));
		port_stream >> port;
		return;
	}
	isAllDigits = true;
	i = 0;
	while (i < addr.length())
	{
		if (!std::isdigit(addr[i]))
		{
			isAllDigits = false;
			break;
		}
		++i;
	}
	if (isAllDigits)
	{
		host = WebServ::DEFAULT_HOST;
		std::istringstream port_stream(addr);
		port_stream >> port;
	}
	else
	{
		host = addr;
		port = WebServ::DEFAULT_PORT;
	}
}

/**
 * The htonl() and htons() functions shall return the argument value 
 * converted from host to network byte order.
 *
 * The ntohl() and ntohs() functions shall return the argument value 
 * converted from network to host byte order.
 *
 * # The problem:
 *
 * Different computer architectures store multi-byte numbers (like int, short) 
 * in different orders:
 *
 * 32-bit number 0x12345678 stored at address 0x1000:
 *
 * LITTLE-ENDIAN (x86, x86_64, most PCs):
 * Address: 0x1000  0x1001  0x1002  0x1003
 * Value:   0x78    0x56    0x34    0x12
 *
 * BIG-ENDIAN (network standard, some servers):
 * Address: 0x1000  0x1001  0x1002  0x1003
 * Value:   0x12    0x34    0x56    0x78
 *
 * If a little-endian machine sends 0x12345678 directly over the network, 
 * a big-endian machine reading those bytes would 
 * interpret it as 0x78563412.
 *
 * # The solution:
 *
 * The network protocol standardizes on Big-Endian order 
 * (also called "network byte order"). All multi-byte values 
 * sent over TCP/IP must be in Big-Endian.
 *
 * HOST to NETWORK (for sending)
 *
 * uint32_t htonl(uint32_t hostlong);   32-bit: IP addresses
 * uint16_t htons(uint16_t hostshort);  16-bit: port numbers
 *
 * NETWORK to HOST (for receiving)
 *
 * uint32_t ntohl(uint32_t netlong);    32-bit: IP addresses
 * uint16_t ntohs(uint16_t netshort);   16-bit: port numbers
 *
 * # About SO_REUSEADDR:
 *
 * When the server crashes or is restarted, the operating system doesn't 
 * immediately free up the port. The socket enters a TIME_WAIT state 
 * for 30-120 seconds.
 *
 * This creates a problem, when the server attempts to start again, 
 * bind() fails because the socket is in TIME_WAIT state.
 *
 * To fix this problem, the SO_REUSEADDR option allows to reuse 
 * the socket immediately.
 *
 * When a TCP connection closes:
 * - The server sends FIN packet
 * - OS keeps the port reserved for TIME_WAIT period
 * - This prevents old, delayed packets from being delivered to a new server
 *
 * SO_REUSEADDR instructs the OS: 
 * "Allow to reuse this port even if it's in TIME_WAIT"
 */
void Server::bindSocket()
{
	std::ostringstream	port_stream;
	struct addrinfo*	gai_result;
	struct addrinfo*	copy;
	struct addrinfo		hints;
	std::string			listenAddress;
	std::string			portStr;
	int					error_code;
	int					bindFd;
	bool				isBound;
	int					option_value;

	if (config.getListenAddresses().empty())
	{
		host = WebServ::DEFAULT_HOST;
		port = WebServ::DEFAULT_PORT;
	}
	else
	{
		listenAddress = config.getListenAddresses()[0];
		parseListenAddress(listenAddress, host, port);
	}
	std::memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	port_stream << port;
	portStr = port_stream.str();
	error_code = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &gai_result);
	if (error_code != 0)
	{
		std::cerr << "Error: getaddrinfo failed: " << gai_strerror(error_code) << std::endl;
		std::exit(EXIT_FAILURE);
	}
	copy = gai_result;
	isBound = false;
	while (copy != NULL && !isBound)
	{
		bindFd = socket(copy->ai_family, copy->ai_socktype, copy->ai_protocol);
		if (bindFd == -1)
		{
			copy = copy->ai_next;
			continue;
		}
		option_value = 1;
		if (setsockopt(bindFd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) == -1)
		{
			std::cerr << "Error: setsockopt SO_REUSEADDR failed" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		if (bind(bindFd, copy->ai_addr, copy->ai_addrlen) == 0)
		{
			listenFd = bindFd;
			makeNonBlocking(listenFd);
			isBound = true;
			break;
		}
		close(bindFd);
		copy = copy->ai_next;
	}
	freeaddrinfo(gai_result);
	if (!isBound)
	{
		std::cerr << "Error: bind failed on " << host << ":" << port << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

/**
 *
 * Web Browser                    Web Server
 *   |                              |
 *   | 1. SYN (I want to connect)   |
 *   |----------------------------->|
 *   |                              |
 *   | 2. SYN-ACK (OK, let's wait)  | ← Connection enters BACKLOG QUEUE
 *   |<-----------------------------|   (not yet accepted by accept())
 *   |                              |
 *   | 3. ACK (Con. established)    |
 *   |----------------------------->|
 *   |                              |
 *   |                              | 4. accept() removes from queue
 *   |                              |    and creates client socket
 *   |<--- HTTP Response -----------|
 */
void Server::startListening()
{
	if (listen(listenFd, WebServ::CONNECTION_BACKLOG) == -1)
	{
		std::cerr << "Error: listen failed" << std::endl;
		close(listenFd);
		std::exit(EXIT_FAILURE);
	}
	std::cout << "[Server] Listening on " << host << ":" << port << std::endl;
}

void Server::addToPoll(int fd, short events)
{
	struct pollfd	pfd;

	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	pollFds.push_back(pfd);
}

void Server::removeFromPoll(int fd)
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

/**
 * struct sockaddr_in is declared so that client 
 * information (IP address, port) can be extracted from it.
 */
void Server::acceptNewConnection()
{
	struct sockaddr_in	clientAddr;
	socklen_t			addrLen;
	int					clientFd;

	addrLen = sizeof(clientAddr);
	clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd == -1)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			std::cerr << "Error: accept failed" << std::endl;
		return;
	}
	makeNonBlocking(clientFd);
	addToPoll(clientFd, POLLIN);
	std::cout << "[Server] New connection from fd " << clientFd << std::endl;
}

void Server::closeConnection(int fd)
{
	std::cout << "[Server] Closing connection fd " << fd << std::endl;
	removeFromPoll(fd);
	close(fd);
}

/**
 * There is data to read.
 */
void Server::handlePollin(int fd)
{
	if (fd == listenFd)
		acceptNewConnection();
	else
		std::cout << "[Server] Data ready to read on fd " << fd << std::endl;
}

/**
 * Writing is now possible, though a write larger than the 
 * available space in a socket or pipe will still block
 * unless O_NONBLOCK is set.
 */
void Server::handlePollout(int fd)
{
	std::cout << "[Server] Ready to write on fd " << fd << std::endl;
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
void Server::handlePollError(int fd)
{
	std::cerr << "[Server] Error on fd " << fd << std::endl;
	closeConnection(fd);
}

void Server::setup()
{
	bindSocket();
	if (listenFd == -1)
	{
		std::cerr << "[Server] FATAL: listenFd is -1 after bindSocket()" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	startListening();
	addToPoll(listenFd, POLLIN);
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
void Server::run()
{
	int		ready;
	size_t	i;

	running = true;
	std::cout << "[Server] Entering event loop" << std::endl;
	while (running)
	{
		ready = poll(&pollFds[0], pollFds.size(), -1);
		if (ready == -1)
		{
			if (errno == EINTR)
				continue;
			std::cerr << "Error: poll failed" << std::endl;
			break;
		}
		i = 0;
		while (i < pollFds.size())
		{
			if (pollFds[i].revents & POLLIN)
				handlePollin(pollFds[i].fd);
			if (pollFds[i].revents & POLLOUT)
				handlePollout(pollFds[i].fd);
			if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				handlePollError(pollFds[i].fd);
			++i;
		}
	}
}

void Server::stop()
{
	running = false;
}

int Server::getListenFd() const
{
	return (listenFd);
}
