#include "config/LocationConfig.hpp"

LocationConfig::LocationConfig() : autoindex(false)
{
}

LocationConfig::LocationConfig(LocationConfig const & that)
	: path(that.path),
	  allowedMethods(that.allowedMethods),
	  redirect(that.redirect),
	  root(that.root),
	  autoindex(that.autoindex),
	  indexFile(that.indexFile),
	  uploadStore(that.uploadStore),
	  cgiExtensions(that.cgiExtensions)
{
}

LocationConfig::~LocationConfig()
{
}

LocationConfig & LocationConfig::operator=(LocationConfig const & that)
{
	if (this != &that)
	{
		path = that.path;
		allowedMethods = that.allowedMethods;
		redirect = that.redirect;
		root = that.root;
		autoindex = that.autoindex;
		indexFile = that.indexFile;
		uploadStore = that.uploadStore;
		cgiExtensions = that.cgiExtensions;
	}
	return (*this);
}

void LocationConfig::setPath(std::string const & newPath)
{
	path = newPath;
}

void LocationConfig::addAllowedMethod(std::string const & method)
{
	allowedMethods.push_back(method);
}

void LocationConfig::setRedirect(std::string const & newRedirect)
{
	redirect = newRedirect;
}

void LocationConfig::setRoot(std::string const & newRoot)
{
	root = newRoot;
}

void LocationConfig::setAutoindex(bool newAutoindex)
{
	autoindex = newAutoindex;
}

void LocationConfig::setIndex(std::string const & newIndex)
{
	indexFile = newIndex;
}

void LocationConfig::setUploadStore(std::string const & newUploadStore)
{
	uploadStore = newUploadStore;
}

void LocationConfig::addCgiExtension(std::string const & ext)
{
	cgiExtensions.push_back(ext);
}

std::string const & LocationConfig::getPath() const
{
	return (path);
}

std::vector<std::string> const & LocationConfig::getAllowedMethods() const
{
	return (allowedMethods);
}

std::string const & LocationConfig::getRedirect() const
{
	return (redirect);
}

std::string const & LocationConfig::getRoot() const
{
	return (root);
}

bool LocationConfig::getAutoindex() const
{
	return (autoindex);
}

std::string const & LocationConfig::getIndex() const
{
	return (indexFile);
}

std::string const & LocationConfig::getUploadStore() const
{
	return (uploadStore);
}

std::vector<std::string> const & LocationConfig::getCgiExtensions() const
{
	return (cgiExtensions);
}

bool LocationConfig::hasRedirect() const
{
	return (!redirect.empty());
}

bool LocationConfig::hasRoot() const
{
	return (!root.empty());
}
