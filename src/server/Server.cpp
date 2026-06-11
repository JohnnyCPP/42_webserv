#include "server/Server.hpp"
#include "constants.hpp"
#include "log/log.hpp"

Server::Server()
	: config(),
	  listenFd(-1),
	  host(""),
	  port(0)
{
}

Server::Server(const Server & that)
	: config(that.config),
	  listenFd(that.listenFd),
	  host(that.host),
	  port(that.port)
{
}

Server::Server(const ServerConfig & config)
	: config(config),
	  listenFd(-1),
	  host(""),
	  port(0)
{
}

Server::~Server()
{
}

Server &	Server::operator=(const Server & that)
{
	if (this != &that)
	{
		config = that.config;
		listenFd = that.listenFd;
		host = that.host;
		port = that.port;
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
		logError(std::string("fcntl() F_GETFL failed: ") + strerror(errno));
		std::exit(EXIT_FAILURE);
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		logError(std::string("fcntl() F_SETFL O_NONBLOCK failed: ") + strerror(errno));
		std::exit(EXIT_FAILURE);
	}
}

void Server::parseListenAddress(const std::string & addr, std::string & host, int & port)
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
	std::ostringstream	info_stream;
	std::ostringstream	error_stream;
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
	info_stream << "a server is creating a listening socket on host " << host << " port " << port;
	log(info_stream.str());
	std::memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	port_stream << port;
	portStr = port_stream.str();
	error_code = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &gai_result);
	if (error_code != 0)
	{
		logError(std::string("getaddrinfo() failed: ") + gai_strerror(error_code));
		std::exit(EXIT_FAILURE);
	}
	copy = gai_result;
	isBound = false;
	while (copy != NULL && !isBound)
	{
		bindFd = socket(copy->ai_family, copy->ai_socktype, copy->ai_protocol);
		if (bindFd == -1)
		{
			logError(std::string("socket() failed: ") + strerror(errno));
			copy = copy->ai_next;
			continue;
		}
		info_stream.str("");
		info_stream.clear();
		info_stream << "a server created a socket with fd " << bindFd;
		log(info_stream.str());
		option_value = 1;
		if (setsockopt(bindFd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) == -1)
		{
			logError(std::string("setsockopt() SO_REUSEADDR failed: ") + strerror(errno));
			std::exit(EXIT_FAILURE);
		}
		if (bind(bindFd, copy->ai_addr, copy->ai_addrlen) == 0)
		{
			listenFd = bindFd;
			makeNonBlocking(listenFd);
			isBound = true;
			info_stream.str("");
			info_stream.clear();
			info_stream << "the server named a socket whose fd is " << bindFd;
			log(info_stream.str());
			break;
		}
		else
		{
			logError(std::string("bind() failed: ") + strerror(errno));
			close(bindFd);
		}
		copy = copy->ai_next;
	}
	freeaddrinfo(gai_result);
	if (!isBound)
	{
		error_stream << "bind failed on " << host << ":" << port;
		logError(error_stream.str());
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
	std::ostringstream	stream;

	if (listen(listenFd, WebServ::CONNECTION_BACKLOG) == -1)
	{
		logError(std::string("listen() failed: ") + strerror(errno));
		close(listenFd);
		std::exit(EXIT_FAILURE);
	}
	stream << "a server is listening on " << host << ":" << port;
	log(stream.str());
}

const ServerConfig&	Server::getConfig() const
{
	return (config);
}

/**
 * struct sockaddr_in is declared so that client 
 * information (IP address, port) can be extracted from it.
 */
int Server::acceptConnection()
{
	struct sockaddr_in	clientAddr;
	std::ostringstream	error_stream;
	std::ostringstream	info_stream;
	socklen_t			addrLen;
	int					clientFd;

	addrLen = sizeof(clientAddr);
	clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd == -1)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			error_stream << "accept() failed on FD " << listenFd << ": ";
			logError(error_stream.str() + strerror(errno));
		}
		return (-1);
	}
	makeNonBlocking(clientFd);
	info_stream << "a server with listening socket " << listenFd
		<< " received a new connection from socket " << clientFd;
	log(info_stream.str());
	return (clientFd);
}

void Server::setup()
{
	bindSocket();
	if (listenFd == -1)
	{
		logError("listening socket of a server is invalid after bindSocket()");
		std::exit(EXIT_FAILURE);
	}
	startListening();
}

int Server::getListenFd() const
{
	return (listenFd);
}
