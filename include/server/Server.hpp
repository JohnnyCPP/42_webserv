#ifndef SERVER_HPP
# define SERVER_HPP

# include "webserv.hpp"
# include "config/ServerConfig.hpp"

class Server
{
private:

	ServerConfig				config;
	int							listenFd;
	std::string					host;
	int							port;

	void				parseListenAddress(const std::string & addr, std::string & outHost, int & outPort);
	void				bindSocket();
	void				makeNonBlocking(int fd);
	void				startListening();

public:

	Server();
	~Server();
	Server(const Server & that);
	Server(const ServerConfig & config);
	Server & operator=(const Server & that);

	void				setup();
	int					getListenFd() const;
	const ServerConfig&	getConfig() const;
	int					acceptConnection();
};

#endif
