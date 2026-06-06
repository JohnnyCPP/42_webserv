#include "webserv.hpp"
#include "config/Config.hpp"
#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"

int	main(int argc, char **argv)
{
	std::string	configPath;
	Config		config;
	size_t		i;
	size_t		j;

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
	std::vector<ServerConfig> const & servers = config.getServers();
	std::cout << "[webserv] Parsed " << servers.size() << " server(s)" << std::endl;
	i = 0;
	while (i < servers.size())
	{
		std::cout << "  Server " << i << ":" << std::endl;
		std::cout << "    listen: ";
		j = 0;
		while (j < servers[i].getListenAddresses().size())
		{
			std::cout << servers[i].getListenAddresses()[j] << " ";
			++j;
		}
		std::cout << std::endl;
		std::cout << "    root: " << servers[i].getRoot() << std::endl;
		std::cout << "    locations: " << servers[i].getLocations().size() << std::endl;
		++i;
	}
	return (EXIT_SUCCESS);
}
