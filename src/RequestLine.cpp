#include "RequestLine.hpp"

RequestLine::RequestLine() : method(), path(), query(), version()
{
}

RequestLine::RequestLine(RequestLine const & that)
	: method(that.method), path(that.path), query(that.query),
	  version(that.version)
{
}

RequestLine::~RequestLine()
{
}

RequestLine & RequestLine::operator=(RequestLine const & that)
{
	if (this != &that)
	{
		method = that.method;
		path = that.path;
		query = that.query;
		version = that.version;
	}
	return (*this);
}

void RequestLine::setMethod(std::string const & value)
{
	method = value;
}

void RequestLine::setPath(std::string const & value)
{
	path = value;
}

void RequestLine::setQuery(std::string const & value)
{
	query = value;
}

void RequestLine::setVersion(std::string const & value)
{
	version = value;
}

std::string const & RequestLine::getMethod() const
{
	return (method);
}

std::string const & RequestLine::getPath() const
{
	return (path);
}

std::string const & RequestLine::getQuery() const
{
	return (query);
}

std::string const & RequestLine::getVersion() const
{
	return (version);
}
