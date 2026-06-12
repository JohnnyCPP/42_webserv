#include "log/log.hpp"

void	log(const std::string & message)
{
	std::cout << WebServ::WS_LOG << " " << message << std::endl;
}

void	logError(const std::string & message)
{
	std::cerr << WebServ::WS_LOG_ERR << " " << message << std::endl;
}

void	logConfig(const Config & config)
{
	const std::vector<ServerConfig> &			servers = config.getServers();
	std::map<int, std::string>::const_iterator	errorIt;
	std::vector<LocationConfig>::const_iterator	locIt;
	std::vector<std::string>::const_iterator	methodIt;
	std::vector<std::string>::const_iterator	cgiIt;
	std::ostringstream							stream;
	size_t										bodySize;
	size_t										bodySizeMB;
	size_t										bodySizeKB;
	size_t										i;
	size_t										j;

	stream << "total servers: " << servers.size();
	log(stream.str());
	i = 0;
	while (i < servers.size())
	{
		stream.str("");
		stream.clear();
		stream << "logging configuration of server " << i;
		log(stream.str());
		stream.str("");
		stream.clear();
		stream << "listen addresses: ";
		j = 0;
		while (j < servers[i].getListenAddresses().size())
		{
			if (j > 0)
				stream << ", ";
			stream << servers[i].getListenAddresses()[j];
			++j;
		}
		log(stream.str());
		log(std::string("server name: ") + servers[i].getServerName());
		bodySize = servers[i].getClientMaxBodySize();
		stream.str("");
		stream.clear();
		stream << "client max body size: " << bodySize << " bytes";
		if (bodySize >= 1048576)
		{
			bodySizeMB = bodySize / 1048576;
			stream << " (" << bodySizeMB << "MB)";
		}
		else if (bodySize >= 1024)
		{
			bodySizeKB = bodySize / 1024;
			stream << " (" << bodySizeKB << "KB)";
		}
		log(stream.str());
		log(std::string("root directory: ") + servers[i].getRoot());
		log(std::string("index file: ") + servers[i].getIndex());
		log("error pages:");
		errorIt = servers[i].getErrorPages().begin();
		while (errorIt != servers[i].getErrorPages().end())
		{
			stream.str("");
			stream.clear();
			stream << errorIt->first << " -> " << errorIt->second;
			log(stream.str());
			++errorIt;
		}
		stream.str("");
		stream.clear();
		stream << "locations (" << servers[i].getLocations().size() << "):";
		locIt = servers[i].getLocations().begin();
		while (locIt != servers[i].getLocations().end())
		{
			log(std::string("location: ") + locIt->getPath());
			if (locIt->hasRedirect())
				log(std::string("redirect: ") + locIt->getRedirect());
			if (locIt->hasRoot())
				log(std::string("custom root: ") + locIt->getRoot());
			if (locIt->getAutoindex())
				log("autoindex: on");
			else
				log("autoindex: off");
			if (!locIt->getIndex().empty())
				log(std::string("index file: ") + locIt->getIndex());
			if (!locIt->getUploadStore().empty())
				log(std::string("upload store: ") + locIt->getUploadStore());
			stream.str("");
			stream.clear();
			stream << "allowed methods: ";
			methodIt = locIt->getAllowedMethods().begin();
			if (methodIt == locIt->getAllowedMethods().end())
				stream << "(none)";
			while (methodIt != locIt->getAllowedMethods().end())
			{
				if (methodIt != locIt->getAllowedMethods().begin())
					stream << ", ";
				stream << *methodIt;
				++methodIt;
			}
			log(stream.str());
			if (!locIt->getCgiExtensions().empty())
			{
				stream.str("");
				stream.clear();
				stream << "CGI extensions: ";
				cgiIt = locIt->getCgiExtensions().begin();
				while (cgiIt != locIt->getCgiExtensions().end())
				{
					if (cgiIt != locIt->getCgiExtensions().begin())
						stream << ", ";
					stream << *cgiIt;
					++cgiIt;
				}
				log(stream.str());
			}
			++locIt;
		}
		++i;
	}
}
