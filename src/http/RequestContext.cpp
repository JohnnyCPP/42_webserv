#include "http/RequestContext.hpp"

RequestContext::RequestContext()
	: matchedLocation(NULL),
	  targetServer(NULL),
	  requestPath(""),
	  resolvedPath("")
{
}

RequestContext::~RequestContext()
{
}

RequestContext::RequestContext(const RequestContext & that)
	: matchedLocation(that.matchedLocation),
	  targetServer(that.targetServer),
	  requestPath(that.requestPath),
	  resolvedPath(that.resolvedPath)
{
}

RequestContext &	RequestContext::operator=(const RequestContext & that)
{
	if (this != &that)
	{
		matchedLocation = that.matchedLocation;
		targetServer = that.targetServer;
		requestPath = that.requestPath;
		resolvedPath = that.resolvedPath;
	}
	return (*this);
}

void	RequestContext::setRequestPath(const std::string & path)
{
	requestPath = path;
}

void	RequestContext::setResolvedPath(const std::string & path)
{
	resolvedPath = path;
}

void	RequestContext::setMatchedLocation(const LocationConfig * location)
{
	matchedLocation = location;
}

void	RequestContext::setTargetServer(const ServerConfig * server)
{
	targetServer = server;
}

const std::string &	RequestContext::getRequestPath() const
{
	return (requestPath);
}

const std::string &	RequestContext::getResolvedPath() const
{
	return (resolvedPath);
}

const LocationConfig *	RequestContext::getMatchedLocation() const
{
	return (matchedLocation);
}

const ServerConfig *	RequestContext::getTargetServer() const
{
	return (targetServer);
}

bool	RequestContext::hasRedirect() const
{
	bool	result;

	if (matchedLocation == NULL)
		result = false;
	else
		result = matchedLocation->hasRedirect();
	return (result);
}

bool	RequestContext::hasCustomRoot() const
{
	bool	result;

	if (matchedLocation == NULL)
		result = false;
	else
		result = matchedLocation->hasRoot();
	return (result);
}
