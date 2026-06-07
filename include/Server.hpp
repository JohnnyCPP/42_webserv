#ifndef SERVER_HPP
# define SERVER_HPP

# include "webserv.hpp"
# include "config/ServerConfig.hpp"

class Server
{
public:

	Server(ServerConfig const & config);
	Server(Server const & that);
	~Server();
	Server & operator=(Server const & that);

	void	setup();
	void	run();
	void	stop();
	int		getListenFd() const;

private:

	ServerConfig				config;
	int							listenFd;
	std::vector<struct pollfd>	pollFds;
	bool						running;
	std::string					host;
	int							port;

	Server();

	void	parseListenAddress(std::string const & addr, std::string & outHost, int & outPort);
	void	bindSocket();
	void	makeNonBlocking(int fd);
	void	startListening();
	void	addToPoll(int fd, short events);
	void	removeFromPoll(int fd);
	void	acceptNewConnection();
	void	handlePollin(int fd);
	void	handlePollout(int fd);
	void	handlePollError(int fd);
	void	closeConnection(int fd);
};

#endif
