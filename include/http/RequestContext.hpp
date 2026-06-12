#ifndef REQUEST_CONTEXT_HPP
# define REQUEST_CONTEXT_HPP

# include "webserv.hpp"
# include "config/LocationConfig.hpp"
# include "config/ServerConfig.hpp"

/**
 * Data carrier that holds all information about a request 
 * as it flows through different processing stages.
 *
 * Instead of passing many parameters between functions, 
 * package everything into one object.
 */

class RequestContext
{
private:

	const LocationConfig *		matchedLocation;
	const ServerConfig *		targetServer;
	std::string					requestPath;
	std::string					resolvedPath;

public:

	RequestContext();
	~RequestContext();
	RequestContext(RequestContext const & that);
	RequestContext & operator=(RequestContext const & that);

	void						setRequestPath(const std::string & path);
	void						setResolvedPath(const std::string & path);
	void						setMatchedLocation(LocationConfig const * location);
	void						setTargetServer(ServerConfig const * server);

	const std::string &			getRequestPath() const;
	const std::string &			getResolvedPath() const;
	const LocationConfig *		getMatchedLocation() const;
	const ServerConfig *		getTargetServer() const;

	bool						hasRedirect() const;
	bool						hasCustomRoot() const;
};

#endif
