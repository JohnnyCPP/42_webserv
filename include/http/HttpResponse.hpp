#ifndef HTTP_RESPONSE_HPP
# define HTTP_RESPONSE_HPP

# include "webserv.hpp"
# include "constants.hpp"
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
	std::string getDefaultBody(int code) const;
	std::string getContentType(const std::string & path) const;
	std::string getStatusLine() const;
	std::string getHeadersString() const;

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

	static HttpResponse	ok(const std::string & body);
	static HttpResponse created(const std::string & location);
	static HttpResponse noContent();
	static HttpResponse movedPermanently(const std::string & location);
	static HttpResponse found(const std::string & location);
	static HttpResponse badRequest();
	static HttpResponse forbidden();
	static HttpResponse notFound();
	static HttpResponse methodNotAllowed(const std::string & allowedMethods);
	static HttpResponse payloadTooLarge();
	static HttpResponse internalServerError();
	static HttpResponse notImplemented();
	static HttpResponse versionNotSupported();
};

#endif
