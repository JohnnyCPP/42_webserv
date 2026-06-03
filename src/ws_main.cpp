#include "Webserv.hpp"

int	main(int argc, char **argv)
{
	std::string	configPath;
	int			listenFd;

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

	// TODO: parse the configuration file
	// TODO: create listening sockets for every interface:port pair from parsed config
	listenFd = ws_create_listen_socket(Http::DEFAULT_HOST, Http::DEFAULT_PORT);
	if (listenFd == -1)
		return (EXIT_FAILURE);
	std::cout << "[webserv] listening on " << Http::DEFAULT_HOST
		<< ":" << Http::DEFAULT_PORT << " (fd=" << listenFd << ")" << std::endl;

	// TODO: run the single-poll() event loop
	close(listenFd);
	return (EXIT_SUCCESS);
}
