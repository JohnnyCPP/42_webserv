#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "webserv.hpp"

class LocationConfig;

class ServerConfig
{
private:

	std::vector<std::string>	listenAddresses;
	std::string					serverName;
	size_t						clientMaxBodySize;
	std::map<int, std::string>	errorPages;
	std::string					root;
	std::string					indexFile;
	std::vector<LocationConfig>	locations;

public:

	ServerConfig();
	~ServerConfig();
	ServerConfig(const ServerConfig & that);
	ServerConfig & operator=(const ServerConfig & that);

	void								addListenAddress(const std::string & address);
	void								setListenAddresses(const std::vector<std::string> & addresses);
	void								setServerName(const std::string & name);
	void								setClientMaxBodySize(size_t size);
	void								addErrorPage(int code, const std::string & path);
	void								setRoot(const std::string & newRoot);
	void								setIndex(const std::string & newIndex);
	void								addLocation(const LocationConfig & location);

	const std::vector<std::string> &	getListenAddresses() const;
	const std::string &					getServerName() const;
	size_t								getClientMaxBodySize() const;
	const std::map<int, std::string> &	getErrorPages() const;
	const std::string &					getRoot() const;
	const std::string &					getIndex() const;
	const std::vector<LocationConfig> &	getLocations() const;

	const LocationConfig *				matchLocation(const std::string & requestPath) const;
};

#endif
