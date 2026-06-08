#include "webserv.hpp"
#include "config/Config.hpp"
#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"
#include "server/WebServer.hpp"

static void printConfigDetails(Config const & config)
{
	std::vector<ServerConfig> const & servers = config.getServers();
	size_t i;
	size_t j;
	std::map<int, std::string>::const_iterator errorIt;
	std::vector<LocationConfig>::const_iterator locIt;
	std::vector<std::string>::const_iterator methodIt;
	std::vector<std::string>::const_iterator cgiIt;
	size_t bodySize;
	size_t bodySizeMB;
	size_t bodySizeKB;

	i = 0;
	std::cout << "\n=== WEBSERV CONFIGURATION PARSED ===" << std::endl;
	std::cout << "Total servers: " << servers.size() << "\n" << std::endl;
	while (i < servers.size())
	{
		std::cout << "┌─────────────────────────────────────────" << std::endl;
		std::cout << "│ SERVER [" << i << "]" << std::endl;
		std::cout << "├─────────────────────────────────────────" << std::endl;
		std::cout << "│ Listen addresses: ";
		j = 0;
		while (j < servers[i].getListenAddresses().size())
		{
			if (j > 0)
				std::cout << ", ";
			std::cout << servers[i].getListenAddresses()[j];
			++j;
		}
		std::cout << std::endl;
		std::cout << "│ Server name: " << servers[i].getServerName() << std::endl;
		bodySize = servers[i].getClientMaxBodySize();
		std::cout << "│ Client max body size: " << bodySize << " bytes";
		if (bodySize >= 1048576)
		{
			bodySizeMB = bodySize / 1048576;
			std::cout << " (" << bodySizeMB << "MB)";
		}
		else if (bodySize >= 1024)
		{
			bodySizeKB = bodySize / 1024;
			std::cout << " (" << bodySizeKB << "KB)";
		}
		std::cout << std::endl;
		std::cout << "│ Root directory: " << servers[i].getRoot() << std::endl;
		std::cout << "│ Index file: " << servers[i].getIndex() << std::endl;
		std::cout << "│ Error pages:" << std::endl;
		std::cout << "│" << std::endl;
		errorIt = servers[i].getErrorPages().begin();
		while (errorIt != servers[i].getErrorPages().end())
		{
			std::cout << "│   " << errorIt->first << " -> " << errorIt->second << std::endl;
			++errorIt;
		}
		std::cout << "│" << std::endl;
		std::cout << "│ Locations (" << servers[i].getLocations().size() << "):" << std::endl;
		locIt = servers[i].getLocations().begin();
		while (locIt != servers[i].getLocations().end())
		{
			std::cout << "│" << std::endl;
			std::cout << "│   ┌─ LOCATION: " << locIt->getPath() << std::endl;
			if (locIt->hasRedirect())
				std::cout << "│   ├─ Redirect: " << locIt->getRedirect() << std::endl;
			if (locIt->hasRoot())
				std::cout << "│   ├─ Custom root: " << locIt->getRoot() << std::endl;
			std::cout << "│   ├─ Autoindex: ";
			if (locIt->getAutoindex())
				std::cout << "ON";
			else
				std::cout << "OFF";
			std::cout << std::endl;
			if (!locIt->getIndex().empty())
				std::cout << "│   ├─ Index file: " << locIt->getIndex() << std::endl;
			if (!locIt->getUploadStore().empty())
				std::cout << "│   ├─ Upload store: " << locIt->getUploadStore() << std::endl;
			std::cout << "│   ├─ Allowed methods: ";
			methodIt = locIt->getAllowedMethods().begin();
			if (methodIt == locIt->getAllowedMethods().end())
				std::cout << "(none)";
			while (methodIt != locIt->getAllowedMethods().end())
			{
				if (methodIt != locIt->getAllowedMethods().begin())
					std::cout << ", ";
				std::cout << *methodIt;
				++methodIt;
			}
			std::cout << std::endl;
			if (!locIt->getCgiExtensions().empty())
			{
				std::cout << "│   └─ CGI extensions: ";
				cgiIt = locIt->getCgiExtensions().begin();
				while (cgiIt != locIt->getCgiExtensions().end())
				{
					if (cgiIt != locIt->getCgiExtensions().begin())
						std::cout << ", ";
					std::cout << *cgiIt;
					++cgiIt;
				}
				std::cout << std::endl;
			}
			else if (!locIt->hasRedirect() && !locIt->hasRoot())
				std::cout << "│   └─ (using server defaults)" << std::endl;
			else
				std::cout << "│   └─ (not using server defaults)" << std::endl;
			++locIt;
		}
		std::cout << "└─────────────────────────────────────────" << std::endl;
		std::cout << std::endl;
		++i;
	}
	std::cout << "=== END OF CONFIGURATION ===" << std::endl;
}

int	main(int argc, char **argv)
{
	std::string	configPath;
	Config		config;

	if (argc > 2)
	{
		std::cerr << "Usage: ./webserv [path_to_config]" << std::endl;
		return (EXIT_FAILURE);
	}
	if (argc == 2)
		configPath = argv[1];
	else
		configPath = WebServ::DEFAULT_CONFIG_PATH;
	std::cout << "[webserv] starting with config: " << configPath << std::endl;
	config.parse(configPath);
	printConfigDetails(config);
	WebServer	webServer(config);
	webServer.run();
	return (EXIT_SUCCESS);
}
