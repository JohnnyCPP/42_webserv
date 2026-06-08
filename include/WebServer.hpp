#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

# include "webserv.hpp"
# include "config/Config.hpp"
# include "Server.hpp"

class WebServer
{
private:

	std::vector<Server*>		servers;
	std::vector<struct pollfd>	pollFds;
	bool						running;

	WebServer();
	WebServer(const WebServer & that);
	WebServer&	operator=(const WebServer & that);

	void	addToPoll(int fd, short events);
	void	removeFromPoll(int fd);
	
	void	getListeningSockets();

	void	handlePollIn(struct pollfd current);
	void	handlePollOut(struct pollfd current);
	void	handlePollErr(struct pollfd current);

public:

	WebServer(Config const & config);
	~WebServer();
	
	void	run();
	void	stop();
};

#endif
