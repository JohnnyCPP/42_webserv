#include "Webserv.hpp"

int	main(int argc, char **argv)
{
	std::string	configPath;

	if (argc > 2)
	{
		std::cerr << "Usage: ./webserv [path_to_config]" << std::endl;
		return (EXIT_FAILURE);
	}
	if (argc == 2)
		configPath = argv[1];
	else
		configPath = Config::DEFAULT_CONFIG_PATH;
	std::cout << "[webserv] starting with config: " << configPath << std::endl;
	std::cout << "[webserv] ..." << std::endl;
	// TODO: parse the configuration file
	// TODO: create listening sockets for every interface:port pair
	// TODO: run the single-poll() event loop
	return (EXIT_SUCCESS);
}
