#include "Webserv.hpp"

int	ws_set_nonblocking(int fd)
{
	int	flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return (-1);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return (-1);
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
		return (-1);
	return (0);
}

int	ws_create_listen_socket(const std::string &host, int port)
{
	struct addrinfo		hints;
	struct addrinfo		*res;
	std::ostringstream	oss;
	int					sockfd;
	int					opt;
	int					ret;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	oss << port;
	ret = getaddrinfo(host.c_str(), oss.str().c_str(), &hints, &res);
	if (ret != 0)
	{
		std::cerr << "[webserv] getaddrinfo: " << gai_strerror(ret) << std::endl;
		return (-1);
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1)
	{
		std::cerr << "[webserv] socket: " << std::strerror(errno) << std::endl;
		freeaddrinfo(res);
		return (-1);
	}

	opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::cerr << "[webserv] setsockopt: " << std::strerror(errno) << std::endl;
		close(sockfd);
		freeaddrinfo(res);
		return (-1);
	}

	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
	{
		std::cerr << "[webserv] bind " << host << ":" << port
			<< ": " << std::strerror(errno) << std::endl;
		close(sockfd);
		freeaddrinfo(res);
		return (-1);
	}

	freeaddrinfo(res);

	if (listen(sockfd, Config::CONNECTION_BACKLOG) == -1)
	{
		std::cerr << "[webserv] listen: " << std::strerror(errno) << std::endl;
		close(sockfd);
		return (-1);
	}

	if (ws_set_nonblocking(sockfd) == -1)
	{
		std::cerr << "[webserv] fcntl: " << std::strerror(errno) << std::endl;
		close(sockfd);
		return (-1);
	}

	return (sockfd);
}
