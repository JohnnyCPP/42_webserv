#include "http/HttpResponse.hpp"
#include "log/log.hpp"

HttpResponse::HttpResponse()
	: headers(),
	  body(""),
	  statusMessage("OK"),
	  statusCode(200)
{
}

HttpResponse::~HttpResponse()
{
}

HttpResponse::HttpResponse(const HttpResponse & that)
	: headers(that.headers),
	  body(that.body),
	  statusMessage(that.statusMessage),
	  statusCode(that.statusCode)
{
}

HttpResponse &	HttpResponse::operator=(const HttpResponse & that)
{
	if (this != &that)
	{
		headers = that.headers;
		body = that.body;
		statusMessage = that.statusMessage;
		statusCode = that.statusCode;
	}
	return (*this);
}

void	HttpResponse::setStatus(int code)
{
	statusCode = code;
	statusMessage = getDefaultMessage(code);
}

void	HttpResponse::setStatus(int code, const std::string & message)
{
	statusCode = code;
	statusMessage = message;
}

void	HttpResponse::setHeader(const std::string & key, const std::string & value)
{
	headers[key] = value;
}

void	HttpResponse::setBody(const std::string & body)
{
	std::ostringstream	contentLength;

	this->body = body;
	contentLength << this->body.size();
	headers["Content-Length"] = contentLength.str();
}

void	HttpResponse::setBodyFromFile(const std::string & path)
{
	std::stringstream	buffer;
	std::ifstream		file;
	std::string			line;

	file.open(path.c_str());
	if (!file.is_open())
	{
		setStatus(404);
		setBody(getDefaultBody(404));
		return;
	}
	while (std::getline(file, line))
		buffer << line << "\n";
	file.close();
	setBody(buffer.str());
	setContentType(path);
}

void	HttpResponse::setContentType(const std::string & path)
{
	std::string	extension;
	size_t	dotPos;

	dotPos = path.rfind('.');
	if (dotPos == std::string::npos)
	{
		headers["Content-Type"] = "text/plain";
		return;
	}
	extension = path.substr(dotPos);
	if (extension == ".html" || extension == ".htm")
		headers["Content-Type"] = "text/html";
	else if (extension == ".css")
		headers["Content-Type"] = "text/css";
	else if (extension == ".js")
		headers["Content-Type"] = "application/javascript";
	else if (extension == ".json")
		headers["Content-Type"] = "application/json";
	else if (extension == ".png")
		headers["Content-Type"] = "image/png";
	else if (extension == ".jpg" || extension == ".jpeg")
		headers["Content-Type"] = "image/jpeg";
	else if (extension == ".gif")
		headers["Content-Type"] = "image/gif";
	else if (extension == ".txt")
		headers["Content-Type"] = "text/plain";
	else if (extension == ".pdf")
		headers["Content-Type"] = "application/pdf";
	else
		headers["Content-Type"] = "application/octet-stream";
}

std::string	HttpResponse::toString() const
{
	std::string	result;

	result = getStatusLine();
	result += WebServ::CRLF;
	result += getHeadersString();
	result += WebServ::CRLF;
	result += body;
	return (result);
}

int	HttpResponse::getStatusCode() const
{
	return (statusCode);
}

void	HttpResponse::clear()
{
	headers.clear();
	body.clear();
	statusMessage = "OK";
	statusCode = 200;
}

std::string	HttpResponse::getStatusLine() const
{
	std::ostringstream	line;

	line << WebServ::HTTP_VERSION << " ";
	line << statusCode << " ";
	line << statusMessage;
	return (line.str());
}

std::string	HttpResponse::getHeadersString() const
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string											result;

	it = headers.begin();
	while (it != headers.end())
	{
		result += it->first + ": " + it->second + WebServ::CRLF;
		++it;
	}
	return (result);
}

std::string	HttpResponse::loadErrorPage(int code, const ServerConfig * config) const
{
	std::map<int, std::string>::const_iterator	it;
	std::stringstream							buffer;
	std::ifstream								file;
	std::string									configuredPath;
	std::string									fullPath;
	std::string									line;
	bool										isAbsolute;

	if (config == NULL)
	{
		logError("configuration is missing");
		return (getDefaultBody(code));
	}
	buffer << "webserv is loading a custom error page with code " << code;
	log(buffer.str());
	it = config->getErrorPages().find(code);
	if (it == config->getErrorPages().end())
	{
		logError("custom error page not found");
		return (getDefaultBody(code));
	}
	configuredPath = it->second;
	if (configuredPath.empty())
	{
		logError("custom error page is found, but its path is empty");
		return (getDefaultBody(code));
	}
	log(std::string("configured path is ") + configuredPath);
	log(std::string("configured root is ") + config->getRoot());
	isAbsolute = configuredPath[0] == '/';
	if (isAbsolute)
	{
		log("configured path is absolute");
		fullPath = config->getRoot();
		if (fullPath.empty())
			fullPath = ".";
		if (fullPath[fullPath.length() - 1] == '/')
			fullPath = fullPath.substr(0, fullPath.length() - 1);
		fullPath += configuredPath;
	}
	else
	{
		log("configured path is relative");
		fullPath = config->getRoot();
		if (fullPath.empty())
			fullPath = ".";
		fullPath += "/";
		fullPath += configuredPath;
	}
	log(std::string("loading custom error page from ") + fullPath);
	buffer.str("");
	buffer.clear();
	file.open(fullPath.c_str());
	if (file.is_open())
	{
		while (std::getline(file, line))
			buffer << line << "\n";
		file.close();
		if (!buffer.str().empty())
			return (buffer.str());
		else
			logError("custom error page is empty");
	}
	return (getDefaultBody(code));
}

bool	HttpResponse::fileExists(const std::string & path) const
{
	struct stat	statbuf;

	return (stat(path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
}

const std::map<std::string, std::string> &	HttpResponse::getHeaders() const
{
	return (headers);
}

std::string	HttpResponse::getDefaultMessage(int code) const
{
	switch (code)
	{
		case 200: return ("OK");
		case 201: return ("Created");
		case 204: return ("No Content");
		case 301: return ("Moved Permanently");
		case 302: return ("Found");
		case 400: return ("Bad Request");
		case 403: return ("Forbidden");
		case 404: return ("Not Found");
		case 405: return ("Method Not Allowed");
		case 413: return ("Payload Too Large");
		case 500: return ("Internal Server Error");
		case 501: return ("Not Implemented");
		case 505: return ("HTTP Version Not Supported");
		default: return ("Unknown");
	}
}

std::string	HttpResponse::getDefaultBody(int code) const
{
	std::stringstream	buffer;
	std::string			body;

	buffer << "webserv is loading a default error page with code " << code;
	log(buffer.str());
	body = "<html><head><title>";
	body += getDefaultMessage(code);
	body += "</title></head><body>";
	body += "<h1>";
	body += getDefaultMessage(code);
	body += "</h1><p>";
	switch (code)
	{
		case 400:
			body += "The server could not understand the request due to invalid syntax.";
			break;
		case 403:
			body += "You don't have permission to access this resource.";
			break;
		case 404:
			body += "The requested resource was not found on this server.";
			break;
		case 405:
			body += "The method is not allowed for the requested resource.";
			break;
		case 413:
			body += "The request body is larger than the server is willing to accept.";
			break;
		case 500:
			body += "The server encountered an internal error and could not complete the request.";
			break;
		case 501:
			body += "The server does not support the functionality required to fulfill the request.";
			break;
		case 505:
			body += "The server does not support the HTTP version used in the request.";
			break;
		default:
			body += "An error occurred while processing your request.";
			break;
	}
	body += "</p></body></html>";
	return (body);
}

std::string	HttpResponse::getContentType(const std::string & path) const
{
	std::string	extension;
	size_t		dotPos;

	dotPos = path.rfind('.');
	if (dotPos == std::string::npos)
		return ("text/plain");
	extension = path.substr(dotPos);
	if (extension == ".html" || extension == ".htm")
		return ("text/html");
	if (extension == ".css")
		return ("text/css");
	if (extension == ".js")
		return ("application/javascript");
	if (extension == ".json")
		return ("application/json");
	if (extension == ".png")
		return ("image/png");
	if (extension == ".jpg" || extension == ".jpeg")
		return ("image/jpeg");
	if (extension == ".gif")
		return ("image/gif");
	if (extension == ".txt")
		return ("text/plain");
	if (extension == ".pdf")
		return ("application/pdf");
	return ("application/octet-stream");
}

HttpResponse	HttpResponse::ok(const std::string & body)
{
	HttpResponse	response;

	response.setStatus(200);
	response.setBody(body);
	return (response);
}

HttpResponse	HttpResponse::created(const std::string & location)
{
	HttpResponse	response;

	response.setStatus(201);
	response.setHeader("Location", location);
	return (response);
}

HttpResponse	HttpResponse::noContent()
{
	HttpResponse	response;

	response.setStatus(204);
	return (response);
}

HttpResponse	HttpResponse::movedPermanently(const std::string & location)
{
	HttpResponse	response;

	response.setStatus(301);
	response.setHeader("Location", location);
	response.setBody(response.getDefaultBody(301));
	return (response);
}

HttpResponse	HttpResponse::found(const std::string & location)
{
	HttpResponse	response;

	response.setStatus(302);
	response.setHeader("Location", location);
	response.setBody(response.getDefaultBody(302));
	return (response);
}

HttpResponse	HttpResponse::badRequest(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(400);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(400, config));
	return (response);
}

HttpResponse	HttpResponse::forbidden(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(403);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(403, config));
	return (response);
}

HttpResponse	HttpResponse::notFound(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(404);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(404, config));
	return (response);
}

HttpResponse	HttpResponse::methodNotAllowed(const std::string & allowedMethods, const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(405);
	response.setHeader("Allow", allowedMethods);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(405, config));
	return (response);
}

HttpResponse	HttpResponse::payloadTooLarge(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(413);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(413, config));
	return (response);
}

HttpResponse	HttpResponse::internalServerError(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(500);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(500, config));
	return (response);
}

HttpResponse	HttpResponse::notImplemented(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(501);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(501, config));
	return (response);
}

HttpResponse	HttpResponse::versionNotSupported(const ServerConfig * config)
{
	HttpResponse	response;

	response.setStatus(505);
	response.setContentType(".html");
	response.setBody(response.loadErrorPage(505, config));
	return (response);
}
