#ifndef CONSTANTS_HPP
# define CONSTANTS_HPP

# include "webserv.hpp"

namespace WebServ
{
	const std::string	AUTOINDEX_OFF = "off";
	const std::string	AUTOINDEX_ON = "on";
	const std::string	BLOCK_CLOSE = "}";
	const std::string	BLOCK_LOCATION = "location";
	const std::string	BLOCK_OPEN = "{";
	const std::string	BLOCK_SERVER = "server";
	const std::string	CRLF = "\r\n";
	const std::string	D_AUTOINDEX = "autoindex";
	const std::string	D_BODY_SIZE = "client_max_body_size";
	const std::string	D_CGI = "cgi_extension";
	const std::string	D_ERROR_PAGE = "error_page";
	const std::string	D_INDEX = "index";
	const std::string	D_LISTEN = "listen";
	const std::string	D_METHODS = "allow_methods";
	const std::string	D_REDIRECT = "return";
	const std::string	D_ROOT = "root";
	const std::string	D_SERVER = "server_name";
	const std::string	D_STORE = "upload_store";
	const std::string	DEFAULT_CONFIG_PATH = "./config/default.conf";
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
