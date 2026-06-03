#ifndef WS_CONSTANTS_HPP
# define WS_CONSTANTS_HPP

namespace Http
{
	const size_t	MAX_BODY_SIZE = 1048576; // 1MB
	const int		DEFAULT_PORT = 8080;
	const int		BUFFER_SIZE = 4096;

	const std::string	DEFAULT_HOST = "0.0.0.0";
	const std::string	SERVER_NAME = "webserv/1.0";
	const std::string	CRLF = "\r\n";
	const std::string	HTTP_VERSION = "HTTP/1.1";
}

namespace Config
{
	const char* const	DEFAULT_CONFIG_PATH = "./config/default.conf";
	const int			MAX_LOCATIONS = 100;
	const int			CONNECTION_BACKLOG = 128;
	const int			RECV_BUFFER_SIZE = 8192;
}

#endif
