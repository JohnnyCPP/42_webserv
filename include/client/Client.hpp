#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "webserv.hpp"

class Client
{
private:

	int									fd;
	std::string							buffer;
	std::string							requestLine;
	std::string							method;
	std::string							path;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	size_t								contentLength;
	bool								headersComplete;
	bool								requestComplete;
	bool								chunked;
	bool								error;

	void	parseRequestLine(const std::string & line);
	void	parseHeaderLine(const std::string & line);
	void	processHeaders();
	void	extractBody();
	bool	isValidMethod(const std::string & method) const;
	bool	isValidVersion(const std::string & version) const;
	void	resetParseState();

public:

	Client();
	~Client();
	Client(const Client & that);
	Client(int fd);
	Client & operator=(const Client & that);

	int											getFd() const;
	void										setFd(int fd);
	const std::string & 						getBuffer() const;
	void										clearBuffer();
	void										appendToBuffer(const std::string & data);
	const std::string & 						getRequestLine() const;
	const std::string & 						getMethod() const;
	const std::string & 						getPath() const;
	const std::string & 						getVersion() const;
	const std::map<std::string, std::string> &	getHeaders() const;
	const std::string & 						getBody() const;
	size_t										getContentLength() const;
	bool										isHeadersComplete() const;
	bool										isRequestComplete() const;
	void										markRequestComplete();
	bool										isChunked() const;
	bool										hasError() const;
	void										setError(bool isError);

	void										parseRequest();
	void										resetForNextRequest();
};

#endif
