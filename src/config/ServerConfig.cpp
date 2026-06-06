#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"

ServerConfig::ServerConfig() : clientMaxBodySize(1048576)
{
}

ServerConfig::ServerConfig(ServerConfig const & that)
	: listenAddresses(that.listenAddresses),
	  serverName(that.serverName),
	  clientMaxBodySize(that.clientMaxBodySize),
	  errorPages(that.errorPages),
	  root(that.root),
	  indexFile(that.indexFile),
	  locations(that.locations)
{
}

ServerConfig::~ServerConfig()
{
}

ServerConfig & ServerConfig::operator=(ServerConfig const & that)
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

void ServerConfig::addListenAddress(std::string const & address)
{
	listenAddresses.push_back(address);
}

void ServerConfig::setServerName(std::string const & name)
{
	serverName = name;
}

void ServerConfig::setClientMaxBodySize(size_t size)
{
	clientMaxBodySize = size;
}

void ServerConfig::addErrorPage(int code, std::string const & path)
{
	errorPages[code] = path;
}

void ServerConfig::setRoot(std::string const & newRoot)
{
	root = newRoot;
}

void ServerConfig::setIndex(std::string const & newIndex)
{
	indexFile = newIndex;
}

void ServerConfig::addLocation(LocationConfig const & location)
{
	locations.push_back(location);
}

std::vector<std::string> const & ServerConfig::getListenAddresses() const
{
	return (listenAddresses);
}

std::string const & ServerConfig::getServerName() const
{
	return (serverName);
}

size_t ServerConfig::getClientMaxBodySize() const
{
	return (clientMaxBodySize);
}

std::map<int, std::string> const & ServerConfig::getErrorPages() const
{
	return (errorPages);
}

std::string const & ServerConfig::getRoot() const
{
	return (root);
}

std::string const & ServerConfig::getIndex() const
{
	return (indexFile);
}

std::vector<LocationConfig> const & ServerConfig::getLocations() const
{
	return (locations);
}

LocationConfig const * ServerConfig::matchLocation(std::string const & requestPath) const
{
	LocationConfig const *	bestMatch;
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
