#ifndef SERVER_HPP
# define SERVER_HPP

# include "webserv.hpp"
# include "config/ServerConfig.hpp"

class Server
{
public:

	Server(ServerConfig const & config);
	~Server();

	void	setup();
	int		getListenFd() const;

	int		acceptConnection();

private:

	ServerConfig				config;
	int							listenFd;
	std::string					host;
	int							port;

	Server();
	Server(Server const & that);
	Server & operator=(Server const & that);

	void	parseListenAddress(std::string const & addr, std::string & outHost, int & outPort);
	void	bindSocket();
	void	makeNonBlocking(int fd);
	void	startListening();
};

#endif
