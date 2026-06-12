#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"

ServerConfig::ServerConfig() 
	: listenAddresses(),
	  serverName(""),
	  clientMaxBodySize(WebServ::MAX_BODY_SIZE),
	  errorPages(),
	  root(""),
	  indexFile(""),
	  locations()
{
}

ServerConfig::~ServerConfig()
{
}

ServerConfig::ServerConfig(const ServerConfig & that)
	: listenAddresses(that.listenAddresses),
	  serverName(that.serverName),
	  clientMaxBodySize(that.clientMaxBodySize),
	  errorPages(that.errorPages),
	  root(that.root),
	  indexFile(that.indexFile),
	  locations(that.locations)
{
}

ServerConfig &	ServerConfig::operator=(const ServerConfig & that)
{
	if (this != &that)
	{
		listenAddresses = that.listenAddresses;
		serverName = that.serverName;
		clientMaxBodySize = that.clientMaxBodySize;
		errorPages = that.errorPages;
		root = that.root;
		indexFile = that.indexFile;
		locations = that.locations;
	}
	return (*this);
}

void	ServerConfig::addListenAddress(const std::string & address)
{
	listenAddresses.push_back(address);
}

void	ServerConfig::setListenAddresses(const std::vector<std::string> & addresses)
{
	listenAddresses = addresses;
}

void	ServerConfig::setServerName(const std::string & name)
{
	serverName = name;
}

void	ServerConfig::setClientMaxBodySize(size_t size)
{
	clientMaxBodySize = size;
}

void	ServerConfig::addErrorPage(int code, const std::string & path)
{
	errorPages[code] = path;
}

void	ServerConfig::setRoot(const std::string & newRoot)
{
	root = newRoot;
}

void	ServerConfig::setIndex(const std::string & newIndex)
{
	indexFile = newIndex;
}

void	ServerConfig::addLocation(const LocationConfig & location)
{
	locations.push_back(location);
}

const std::vector<std::string> &	ServerConfig::getListenAddresses() const
{
	return (listenAddresses);
}

const std::string &	ServerConfig::getServerName() const
{
	return (serverName);
}

size_t	ServerConfig::getClientMaxBodySize() const
{
	return (clientMaxBodySize);
}

const std::map<int, std::string> &	ServerConfig::getErrorPages() const
{
	return (errorPages);
}

const std::string &	ServerConfig::getRoot() const
{
	return (root);
}

const std::string &	ServerConfig::getIndex() const
{
	return (indexFile);
}

const std::vector<LocationConfig> &	ServerConfig::getLocations() const
{
	return (locations);
}

const LocationConfig *	ServerConfig::matchLocation(const std::string & requestPath) const
{
	const LocationConfig *	bestMatch;
	size_t					maxMatched;
	size_t					currentMatched;
	size_t					i;

	bestMatch = NULL;
	maxMatched = 0;
	i = 0;
	while (i < locations.size())
	{
		if (requestPath.find(locations[i].getPath()) == 0)
		{
			currentMatched = locations[i].getPath().length();
			if (currentMatched > maxMatched)
			{
				maxMatched = currentMatched;
				bestMatch = &(locations[i]);
			}
		}
		++i;
	}
	return (bestMatch);
}
