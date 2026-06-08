#include "Server.hpp"
#include "constants.hpp"

Server::Server(ServerConfig const & config) : config(config), listenFd(-1), host(""), port(0)
{
}

Server::~Server()
{
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
	std::cout << "[Debug] parsed listenAddress: " << listenAddress << std::endl;
	std::cout << "[Debug] host: " << host << ", port: " << port << std::endl;
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
	std::cout << "[Debug] getaddrinfo error_code: " << error_code << std::endl;
	copy = gai_result;
	isBound = false;
	while (copy != NULL && !isBound)
	{
		bindFd = socket(copy->ai_family, copy->ai_socktype, copy->ai_protocol);
		if (bindFd == -1)
		{
			std::cerr << "[Debug] socket() failed: " << strerror(errno) << std::endl;
			copy = copy->ai_next;
			continue;
		}
		std::cout << "[Debug] Created socket fd=" << bindFd << std::endl;
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
			std::cout << "[Debug] Bind SUCCESS on fd=" << listenFd << std::endl;
			break;
		}
		else
		{
			std::cerr << "[Debug] bind() failed: " << strerror(errno) << std::endl;
			close(bindFd);
		}
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

/**
 * struct sockaddr_in is declared so that client 
 * information (IP address, port) can be extracted from it.
 */
int Server::acceptConnection()
{
	struct sockaddr_in	clientAddr;
	socklen_t			addrLen;
	int					clientFd;

	addrLen = sizeof(clientAddr);
	clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd == -1)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			std::cerr << "Error: accept failed on FD " << listenFd << std::endl;
		return (-1);
	}
	makeNonBlocking(clientFd);
	std::cout << "[Server] New connection from fd " << clientFd << std::endl;
	return (clientFd);
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
}

int Server::getListenFd() const
{
	return (listenFd);
}
