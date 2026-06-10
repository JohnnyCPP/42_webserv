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
	~LocationConfig();
	LocationConfig(const LocationConfig & that);
	LocationConfig & operator=(const LocationConfig & that);

	void				setPath(const std::string & newPath);
	void				addAllowedMethod(const std::string & method);
	void				setRedirect(const std::string & newRedirect);
	void				setRoot(const std::string & newRoot);
	void				setAutoindex(bool newAutoindex);
	void				setIndex(const std::string & newIndex);
	void				setUploadStore(const std::string & newUploadStore);
	void				addCgiExtension(const std::string & ext);

	const std::string &					getPath() const;
	const std::vector<std::string> &	getAllowedMethods() const;
	const std::string &					getRedirect() const;
	const std::string &					getRoot() const;
	bool								getAutoindex() const;
	const std::string &					getIndex() const;
	const std::string &					getUploadStore() const;
	const std::vector<std::string> &	getCgiExtensions() const;

	bool								hasRedirect() const;
	bool								hasRoot() const;
};

#endif
