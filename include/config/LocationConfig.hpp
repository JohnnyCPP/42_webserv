#ifndef LOCATION_CONFIG_HPP
# define LOCATION_CONFIG_HPP

# include "webserv.hpp"

class LocationConfig
{
private:

	std::string					path;
	std::vector<std::string>	allowedMethods;
	std::string					redirect;
	std::string					root;
	bool						autoindex;
	std::string					indexFile;
	std::string					uploadStore;
	std::vector<std::string>	cgiExtensions;

public:

	LocationConfig();
	LocationConfig(LocationConfig const & that);
	~LocationConfig();
	LocationConfig & operator=(LocationConfig const & that);

	void				setPath(std::string const & newPath);
	void				addAllowedMethod(std::string const & method);
	void				setRedirect(std::string const & newRedirect);
	void				setRoot(std::string const & newRoot);
	void				setAutoindex(bool newAutoindex);
	void				setIndex(std::string const & newIndex);
	void				setUploadStore(std::string const & newUploadStore);
	void				addCgiExtension(std::string const & ext);

	std::string const &					getPath() const;
	std::vector<std::string> const &	getAllowedMethods() const;
	std::string const &					getRedirect() const;
	std::string const &					getRoot() const;
	bool								getAutoindex() const;
	std::string const &					getIndex() const;
	std::string const &					getUploadStore() const;
	std::vector<std::string> const &	getCgiExtensions() const;

	bool								hasRedirect() const;
	bool								hasRoot() const;
};

#endif
