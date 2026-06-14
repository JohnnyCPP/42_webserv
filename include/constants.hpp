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
	const std::string	CGI_CONTENT_LENGTH = "CONTENT_LENGTH";
	const std::string	CGI_CONTENT_TYPE = "CONTENT_TYPE";
	const std::string	CGI_GATEWAY_INTERFACE = "GATEWAY_INTERFACE";
	const std::string	CGI_PATH_INFO = "PATH_INFO";
	const std::string	CGI_QUERY_STRING = "QUERY_STRING";
	const std::string	CGI_REDIRECT_STATUS = "REDIRECT_STATUS";
	const std::string	CGI_REQUEST_METHOD = "REQUEST_METHOD";
	const std::string	CGI_SCRIPT_FILENAME = "SCRIPT_FILENAME";
	const std::string	CGI_SCRIPT_NAME = "SCRIPT_NAME";
	const std::string	CGI_SERVER_NAME = "SERVER_NAME";
	const std::string	CGI_SERVER_PROTOCOL = "SERVER_PROTOCOL";
	const std::string	CGI_SERVER_SOFTWARE = "SERVER_SOFTWARE";
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
	const std::string	DEFAULT_UPLOADS = "./www/demo/upload";
	const std::string	EXTENSION_PHP = ".php";
	const std::string	EXTENSION_PYTHON = ".py";
	const std::string	GATEWAY_INT = "CGI/1.1";
	const std::string	HTTP_VERSION = "HTTP/1.1";
	const std::string	HTTP_VERSION_LEGACY = "HTTP/1.0";
	const std::string	INTERPRETER_PHP = "/usr/bin/php-cgi";
	const std::string	INTERPRETER_PYTHON = "/usr/bin/python3";
	const std::string	PHP_REDIRECT = "200";
	const std::string	SERVER_NAME = "webserv";
	const std::string	WS_LOG = "[webserv]";
	const std::string	WS_LOG_ERR = "[error]";

	const int			CGI_BUFFER = 4096;
	const int			CGI_TIMEOUT = 30;
	const int			CONNECTION_BACKLOG = 128;
	const int			DEFAULT_PORT = 8080;
	const size_t		MAX_BODY_SIZE = 1048576;
	const int			RECV_BUFFER_SIZE = 8192;
}

#endif
