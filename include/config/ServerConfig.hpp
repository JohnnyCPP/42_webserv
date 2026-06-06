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
		ServerConfig(ServerConfig const & that);
		~ServerConfig();
		ServerConfig & operator=(ServerConfig const & that);

		void		addListenAddress(std::string const & address);
		void		setServerName(std::string const & name);
		void		setClientMaxBodySize(size_t size);
		void		addErrorPage(int code, std::string const & path);
		void		setRoot(std::string const & newRoot);
		void		setIndex(std::string const & newIndex);
		void		addLocation(LocationConfig const & location);

		std::vector<std::string> const &	getListenAddresses() const;
		std::string const &					getServerName() const;
		size_t								getClientMaxBodySize() const;
		std::map<int, std::string> const &	getErrorPages() const;
		std::string const &					getRoot() const;
		std::string const &					getIndex() const;
		std::vector<LocationConfig> const &	getLocations() const;

		LocationConfig const *				matchLocation(std::string const & requestPath) const;
};

#endif
