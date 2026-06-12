#include "webserv.hpp"
#include "log/log.hpp"
#include "config/Config.hpp"
#include "server/WebServer.hpp"

volatile sig_atomic_t	g_running = true;

void	signalHandler(int signal)
{
	(void) signal;
	log("received shutdown signal, exiting...");
	g_running = false;
}

int	main(int argc, char **argv)
{
	std::string			configPath;
	Config				config;

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGQUIT, signalHandler);
	if (argc > 2)
	{
		logError("Usage: ./webserv [path_to_config]");
		return (EXIT_FAILURE);
	}
	if (argc == 2)
		configPath = argv[1];
	else
		configPath = WebServ::DEFAULT_CONFIG_PATH;
	config.parse(configPath);
	WebServer	webServer(config);
	webServer.run();
	return (EXIT_SUCCESS);
}
