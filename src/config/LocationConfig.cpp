#include "config/LocationConfig.hpp"

LocationConfig::LocationConfig()
	: path(""),
	  allowedMethods(),
	  redirect(""),
	  root(""),
	  autoindex(false),
	  indexFile(""),
	  uploadStore(""),
	  cgiExtensions()
{
}

LocationConfig::~LocationConfig()
{
}

LocationConfig::LocationConfig(const LocationConfig & that)
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

LocationConfig & LocationConfig::operator=(const LocationConfig & that)
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

void LocationConfig::setPath(const std::string & newPath)
{
	path = newPath;
}

void LocationConfig::addAllowedMethod(const std::string & method)
{
	allowedMethods.push_back(method);
}

void LocationConfig::setRedirect(const std::string & newRedirect)
{
	redirect = newRedirect;
}

void LocationConfig::setRoot(const std::string & newRoot)
{
	root = newRoot;
}

void LocationConfig::setAutoindex(bool newAutoindex)
{
	autoindex = newAutoindex;
}

void LocationConfig::setIndex(const std::string & newIndex)
{
	indexFile = newIndex;
}

void LocationConfig::setUploadStore(const std::string & newUploadStore)
{
	uploadStore = newUploadStore;
}

void LocationConfig::addCgiExtension(const std::string & ext)
{
	cgiExtensions.push_back(ext);
}

const std::string & LocationConfig::getPath() const
{
	return (path);
}

const std::vector<std::string> & LocationConfig::getAllowedMethods() const
{
	return (allowedMethods);
}

const std::string & LocationConfig::getRedirect() const
{
	return (redirect);
}

const std::string & LocationConfig::getRoot() const
{
	return (root);
}

bool LocationConfig::getAutoindex() const
{
	return (autoindex);
}

const std::string & LocationConfig::getIndex() const
{
	return (indexFile);
}

const std::string & LocationConfig::getUploadStore() const
{
	return (uploadStore);
}

const std::vector<std::string> & LocationConfig::getCgiExtensions() const
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
