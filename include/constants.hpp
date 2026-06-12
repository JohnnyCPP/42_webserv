#ifndef CONSTANTS_HPP
# define CONSTANTS_HPP

# include "webserv.hpp"

namespace WebServ
{
	const std::string	CRLF = "\r\n";
	const std::string	DEFAULT_CONFIG_PATH = "./config/complete.conf";
	const std::string	DEFAULT_HOST = "0.0.0.0";
	const std::string	DEFAULT_INDEX = "index.html";
	const std::string	DEFAULT_UPLOADS = "./uploads";
	const std::string	HTTP_VERSION = "HTTP/1.1";
	const std::string	HTTP_VERSION_LEGACY = "HTTP/1.0";
	const std::string	WS_LOG = "[webserv]";
	const std::string	WS_LOG_ERR = "[error]";

	const int			CONNECTION_BACKLOG = 128;
	const int			DEFAULT_PORT = 8080;
	const size_t		MAX_BODY_SIZE = 1048576;
	const int			RECV_BUFFER_SIZE = 8192;
}

#endif
