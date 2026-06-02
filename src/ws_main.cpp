#include <iostream>
#include <string>

#include "Webserv.hpp"

namespace
{
	const char	*kDefaultConfig = "config/default.conf";
}

int	main(int argc, char **argv)
{
	std::string	configPath;

	if (argc > 2)
	{
		std::cerr << "Usage: " << argv[0] << " [configuration file]" << std::endl;
		return (EXIT_FAILURE);
	}

	configPath = (argc == 2) ? argv[1] : kDefaultConfig;

	std::cout << "[webserv] starting with config: " << configPath << std::endl;

	// TODO: parse the configuration file (config parser module).
	// TODO: create listening sockets for every interface:port pair.
	// TODO: run the single-poll() event loop.

	std::cout << "[webserv] ..." << std::endl;
	return (EXIT_SUCCESS);
}
