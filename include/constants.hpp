#ifndef CONSTANTS_HPP
# define CONSTANTS_HPP

# include "webserv.hpp"

namespace WebServ
{
	const size_t		MAX_BODY_SIZE = 1048576; // 1MB
	const int			DEFAULT_PORT = 8080;
	const int			BUFFER_SIZE = 4096;

	const std::string	DEFAULT_HOST = "0.0.0.0";
	const std::string	SERVER_NAME = "webserv/1.0";
	const std::string	CRLF = "\r\n";
	const std::string	HTTP_VERSION = "HTTP/1.1";
	const std::string	HTTP_VERSION_LEGACY = "HTTP/1.0";

	const int			REQUEST_INCOMPLETE = -1;
	const int			OK = 200;
	const int			BAD_REQUEST = 400;
	const int			METHOD_NOT_ALLOWED = 405;
	const int			VERSION_NOT_SUPPORTED = 505;

	const char* const	DEFAULT_CONFIG_PATH = "./config/complete.conf";
	const int			MAX_LOCATIONS = 100;
	const int			CONNECTION_BACKLOG = 128;
	const int			RECV_BUFFER_SIZE = 8192;

	const std::string	WS_LOG = "[webserv]";
	const std::string	WS_LOG_ERR = "[error]";
}

#endif
