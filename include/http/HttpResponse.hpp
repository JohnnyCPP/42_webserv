#ifndef HTTP_RESPONSE_HPP
# define HTTP_RESPONSE_HPP

# include "webserv.hpp"
# include "constants.hpp"
# include "config/ServerConfig.hpp"
# include <fstream>
# include <sstream>

class HttpResponse
{
private:

	std::map<std::string, std::string>	headers;
	std::string							body;
	std::string							statusMessage;
	int									statusCode;

	std::string	getDefaultMessage(int code) const;
	std::string	getDefaultBody(int code) const;
	std::string	getContentType(const std::string & path) const;
	std::string	getStatusLine() const;
	std::string	getHeadersString() const;
	std::string	loadErrorPage(int code, const ServerConfig * config) const;
	bool		fileExists(const std::string & path) const;

public:

	HttpResponse();
	~HttpResponse();
	HttpResponse(const HttpResponse & that);
	HttpResponse & operator=(const HttpResponse & that);

	void				setStatus(int code);
	void				setStatus(int code, const std::string & message);
	void				setHeader(const std::string & key, const std::string & value);
	void				setBody(const std::string & body);
	void				setBodyFromFile(const std::string & path);
	void				setContentType(const std::string & path);
	std::string			toString() const;
	int					getStatusCode() const;
	void				clear();

	const std::map<std::string, std::string> &	getHeaders() const;

	static HttpResponse	ok(const std::string & body);
	static HttpResponse created(const std::string & location);
	static HttpResponse noContent();
	static HttpResponse movedPermanently(const std::string & location);
	static HttpResponse found(const std::string & location);
	static HttpResponse badRequest(const ServerConfig * config);
	static HttpResponse forbidden(const ServerConfig * config);
	static HttpResponse notFound(const ServerConfig * config);
	static HttpResponse methodNotAllowed(const std::string & allowedMethods, const ServerConfig * config);
	static HttpResponse payloadTooLarge(const ServerConfig * config);
	static HttpResponse internalServerError(const ServerConfig * config);
	static HttpResponse notImplemented(const ServerConfig * config);
	static HttpResponse versionNotSupported(const ServerConfig * config);
};

#endif
