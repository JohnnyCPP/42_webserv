#include "constants.hpp"
#include "client/Client.hpp"

Client::Client()
	: fd(-1),
	  buffer(""),
	  requestLine(""),
	  method(""),
	  path(""),
	  version(""),
	  headers(),
	  body(""),
	  contentLength(0),
	  maxBodySize(WebServ::MAX_BODY_SIZE),
	  headersComplete(false),
	  requestComplete(false),
	  chunked(false),
	  error(false),
	  expectedChunkSize(0),
	  readingChunkSize(true),
	  readingChunkData(false),
	  chunkedBody("")
{
}

Client::~Client()
{
}

Client::Client(const Client & that)
	: fd(that.fd),
	  buffer(that.buffer),
	  requestLine(that.requestLine),
	  method(that.method),
	  path(that.path),
	  version(that.version),
	  headers(that.headers),
	  body(that.body),
	  contentLength(that.contentLength),
	  maxBodySize(that.maxBodySize),
	  headersComplete(that.headersComplete),
	  requestComplete(that.requestComplete),
	  chunked(that.chunked),
	  error(that.error),
	  expectedChunkSize(that.expectedChunkSize),
	  readingChunkSize(that.readingChunkSize),
	  readingChunkData(that.readingChunkData),
	  chunkedBody(that.chunkedBody)
{
}

Client::Client(int fd)
	: fd(fd),
	  buffer(""),
	  requestLine(""),
	  method(""),
	  path(""),
	  version(""),
	  headers(),
	  body(""),
	  contentLength(0),
	  maxBodySize(WebServ::MAX_BODY_SIZE),
	  headersComplete(false),
	  requestComplete(false),
	  chunked(false),
	  error(false),
	  expectedChunkSize(0),
	  readingChunkSize(true),
	  readingChunkData(false),
	  chunkedBody("")
{
}

Client &	Client::operator=(const Client & that)
{
	if (this != &that)
	{
		fd = that.fd;
		buffer = that.buffer;
		requestLine = that.requestLine;
		method = that.method;
		path = that.path;
		version = that.version;
		headers = that.headers;
		body = that.body;
		contentLength = that.contentLength;
		maxBodySize = that.maxBodySize;
		headersComplete = that.headersComplete;
		requestComplete = that.requestComplete;
		chunked = that.chunked;
		error = that.error;
		expectedChunkSize = that.expectedChunkSize;
		readingChunkSize = that.readingChunkSize;
		readingChunkData = that.readingChunkData;
		chunkedBody = that.chunkedBody;
	}
	return (*this);
}

void	Client::setFd(int fd)
{
	this->fd = fd;
}

int	Client::getFd() const
{
	return (fd);
}

const std::string &	Client::getBuffer() const
{
	return (buffer);
}

void	Client::appendToBuffer(const std::string & data)
{
	buffer += data;
}

void	Client::clearBuffer()
{
	buffer.clear();
}

const std::string &	Client::getRequestLine() const
{
	return (requestLine);
}

const std::string &	Client::getMethod() const
{
	return (method);
}

const std::string &	Client::getPath() const
{
	return (path);
}

const std::string &	Client::getVersion() const
{
	return (version);
}

const std::map<std::string, std::string> &	Client::getHeaders() const
{
	return (headers);
}

const std::string &	Client::getBody() const
{
	return (body);
}

size_t	Client::getContentLength() const
{
	return (contentLength);
}

void	Client::setMaxBodySize(size_t size)
{
	maxBodySize = size;
}

size_t	Client::getMaxBodySize() const
{
	return (maxBodySize);
}

bool	Client::isBodySizeExceeded() const
{
	bool	result;

	result = (error && contentLength > maxBodySize);
	return (result);
}

bool	Client::isHeadersComplete() const
{
	return (headersComplete);
}

bool	Client::isRequestComplete() const
{
	return (requestComplete);
}

void	Client::markRequestComplete()
{
	requestComplete = true;
}

bool	Client::isChunked() const
{
	return (chunked);
}

bool	Client::hasError() const
{
	return (error);
}

void	Client::setError(bool isError)
{
	error = isError;
}

void	Client::parseRequest()
{
	std::string	line;
	size_t		headerEnd;
	size_t		lineEnd;
	size_t		pos;

	log("request is being parsed");
	if (headersComplete)
	{
		extractBody();
		return;
	}
	headerEnd = buffer.find(WebServ::CRLF + WebServ::CRLF);
	if (headerEnd == std::string::npos)
		return;
	pos = 0;
	lineEnd = buffer.find(WebServ::CRLF, pos);
	if (lineEnd == std::string::npos)
		return;
	requestLine = buffer.substr(pos, lineEnd - pos);
	parseRequestLine(requestLine);
	pos = lineEnd + 2;
	while (pos < headerEnd)
	{
		lineEnd = buffer.find(WebServ::CRLF, pos);
		if (lineEnd == std::string::npos)
			break;
		line = buffer.substr(pos, lineEnd - pos);
		if (!line.empty())
			parseHeaderLine(line);
		pos = lineEnd + 2;
	}
	headersComplete = true;
	processHeaders();
	if (contentLength == 0 && !chunked)
	{
		requestComplete = true;
		return;
	}
	extractBody();
}

void	Client::resetForNextRequest()
{
	requestLine.clear();
	method.clear();
	path.clear();
	version.clear();
	headers.clear();
	body.clear();
	contentLength = 0;
	headersComplete = false;
	requestComplete = false;
	chunked = false;
	error = false;
	expectedChunkSize = 0;
	readingChunkSize = true;
	readingChunkData = false;
	chunkedBody.clear();
	resetParseState();
}

const std::string &	Client::getUnchunkedBody() const
{
	if (chunkedBody.empty())
		return (body);
	return (chunkedBody);
}

/**
 * GET /index.html HTTP/1.1
 *  │     │           │
 *  │     │           └── version
 *  │     └────────────── path
 *  └──────────────────── method
 */
void	Client::parseRequestLine(const std::string & line)
{
	size_t	firstSpace;
	size_t	secondSpace;

	log(std::string("parsing request line: ") + line);
	firstSpace = line.find(' ');
	if (firstSpace == std::string::npos)
	{
		logError("first space not found");
		error = true;
		return;
	}
	secondSpace = line.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
	{
		logError("second space not found");
		error = true;
		return;
	}
	method = line.substr(0, firstSpace);
	path = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	version = line.substr(secondSpace + 1);
	if (!isValidMethod(method))
	{
		logError("method is not valid");
		error = true;
	}
	if (!isValidVersion(version))
	{
		logError("version is not valid");
		error = true;
	}
}

void	Client::parseHeaderLine(const std::string & line)
{
	std::string	key;
	std::string	value;
	size_t		colonPos;

	log(std::string("parsing header line: ") + line);
	colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		error = true;
		return;
	}
	key = line.substr(0, colonPos);
	value = line.substr(colonPos + 1);
	while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
		value.erase(0, 1);
	if (key.empty())
	{
		error = true;
		return;
	}
	headers[key] = value;
}

void	Client::processHeaders()
{
	std::map<std::string, std::string>::iterator	it;
	std::stringstream								stream;
	std::string										encoding;

	it = headers.find("Content-Length");
	if (it != headers.end())
	{
		stream << it->second;
		stream >> contentLength;
	}
	it = headers.find("Transfer-Encoding");
	if (it != headers.end())
	{
		encoding = it->second;
		if (encoding.find("chunked") != std::string::npos)
			chunked = true;
	}
}

void	Client::extractBody()
{
	std::ostringstream	stream;
	std::string			remaining;
	size_t				headerEnd;

	log("request body is being parsed");
	if (requestComplete)
		return;
	headerEnd = buffer.find(WebServ::CRLF + WebServ::CRLF);
	if (headerEnd == std::string::npos)
		return;
	remaining = buffer.substr(headerEnd + 4);
	if (chunked)
	{
		log("request body is chunked");
		parseChunkedBody();
		return;
	}
	else
		log("request body is not chunked");
	if (contentLength > 0)
	{
		if (contentLength > maxBodySize)
		{
			stream << "Content-Length " << contentLength
					<< " is greater than max body size " << maxBodySize;
			logError(stream.str());
			error = true;
			return;
		}
		if (remaining.size() >= contentLength)
		{
			body = remaining.substr(0, contentLength);
			requestComplete = true;
		}
	}
	else
		requestComplete = true;
	log("request parsing complete");
}

bool	Client::isValidMethod(const std::string & method) const
{
	size_t	i;
	char	c;

	i = 0;
	while (i < method.length())
	{
		c = method[i];
		if (!std::isupper(c) && c != '-' && !std::isdigit(c))
			return (false);
		++i;
	}
	return (true);
}

/**
 * If HTTP/1.0, keepAlive = false (close after response)
 * If HTTP/1.1, keepAlive = true (check Connection header for "close")
 * Chunked encoding is not supported in HTTP/1.0 (HTTP/1.1 only)
 */
bool	Client::isValidVersion(const std::string & version) const
{
	return (version == WebServ::HTTP_VERSION);
}

void	Client::resetParseState()
{
	std::string	remaining;
	size_t		headerEnd;
	size_t		consumed;
	size_t		bytesToSkip;

	headerEnd = buffer.find(WebServ::CRLF + WebServ::CRLF);
	if (headerEnd == std::string::npos)
	{
		buffer.clear();
		return;
	}
	remaining = buffer.substr(headerEnd + 4);
	consumed = headerEnd + 4;
	if (chunked) // TODO: handle chunked body
	{
		buffer.clear();
		return;
	}
	if (contentLength > 0)
	{
		bytesToSkip = contentLength;
		if (remaining.size() >= bytesToSkip)
		{
			consumed += bytesToSkip;
			buffer = buffer.substr(consumed);
		}
		else
			buffer.clear();
	}
	else
		buffer = remaining;
}

void	Client::parseChunkedBody()
{
	std::ostringstream	stream;
	std::string			chunkData;
	std::string			remaining;
	std::string			line;
	size_t				headerEnd;
	size_t				lineEnd;
	size_t				pos;

	log("chunked body is being parsed");
	if (requestComplete)
		return;
	headerEnd = buffer.find(WebServ::CRLF + WebServ::CRLF);
	if (headerEnd == std::string::npos)
		return;
	remaining = buffer.substr(headerEnd + 4);
	if (remaining.empty())
		return;
	pos = 0;
	while (pos < remaining.length())
	{
		if (readingChunkSize)
		{
			lineEnd = remaining.find(WebServ::CRLF, pos);
			if (lineEnd == std::string::npos)
				break;
			line = remaining.substr(pos, lineEnd - pos);
			decodeChunkSize(line);
			readingChunkSize = false;
			pos = lineEnd + 2;
			if (expectedChunkSize == 0)
			{
				log("chunked request parsing complete");
				requestComplete = true;
				body = chunkedBody;
				readingChunkSize = true;
				readingChunkData = false;
				chunkedBody.clear();
				return;
			}
			readingChunkData = true;
		}
		if (readingChunkData)
		{
			if (pos + expectedChunkSize + 2 > remaining.length())
				break;
			chunkData = remaining.substr(pos, expectedChunkSize);
			stream << "read chunk data: " << chunkData;
			log(stream.str());
			chunkedBody += chunkData;
			pos += expectedChunkSize;
			if (pos + 2 <= remaining.length() && remaining.substr(pos, 2) == WebServ::CRLF)
				pos += 2;
			else
				break;
			readingChunkSize = true;
			readingChunkData = false;
		}
	}
	if (requestComplete)
		return;
	if (chunkedBody.size() > maxBodySize)
	{
		stream.str("");
		stream.clear();
		stream << "chunked body size " << chunkedBody.size() 
			<< " is greater than max body size " << maxBodySize;
		logError(stream.str());
		error = true;
	}
}

std::string	Client::decodeChunkSize(const std::string & line)
{
	std::ostringstream	stream;
	std::string			hexPart;
	size_t				i;
	size_t				size;
	size_t				pos;

	i = 0;
	while (i < line.length() && isHexDigit(line[i]))
	{
		hexPart += line[i];
		++i;
	}
	size = 0;
	i = 0;
	while (i < hexPart.length())
	{
		size = size * 16 + hexToInt(hexPart[i]);
		++i;
	}
	expectedChunkSize = size;
	stream << "decoded chunk size " << expectedChunkSize;
	log(stream.str());
	pos = line.find(';');
	if (pos != std::string::npos)
		return (line.substr(0, pos));
	return (line);
}

bool	Client::isHexDigit(char c) const
{
	if (c >= '0' && c <= '9')
		return (true);
	if (c >= 'a' && c <= 'f')
		return (true);
	if (c >= 'A' && c <= 'F')
		return (true);
	return (false);
}

int	Client::hexToInt(char c) const
{
	int	result;

	if (c >= '0' && c <= '9')
		result = (c - '0');
	else if (c >= 'a' && c <= 'f')
		result = (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		result = (c - 'A' + 10);
	else
		result = 0;
	return (result);
}
