#ifndef WEB_SERVER_HPP
# define WEB_SERVER_HPP

# include "webserv.hpp"
# include "config/Config.hpp"
# include "server/Server.hpp"
# include "client/Client.hpp"
# include "http/HttpResponse.hpp"
# include "http/RequestContext.hpp"

class WebServer
{
private:

	std::vector<Server>			servers;
	std::vector<struct pollfd>	pollFds;
	std::map<int, Client>		clients;
	std::map<int, Server*>		clientToServer;
	std::map<int, std::string>	pendingResponses;
	std::vector<int>			clientsToRemove;
	bool						running;

	void		addToPoll(int fd, short events);
	void		removeFromPoll(int fd);
	
	void		getListeningSockets();

	void		handlePollIn(struct pollfd current);
	void		handlePollOut(struct pollfd current);
	void		handlePollErr(struct pollfd current);

	void		handleClientRead(int fd);
	void		processClientRequest(int fd);
	void		removeClient(int fd);
	void		cleanupRemovedClients();
	void		modifyPollEvents(int fd, short events);
 
	void		buildRequestContext(int clientFd, RequestContext & context);
	void		resolveFilesystemPath(RequestContext & context);
	void		handleRedirect(const RequestContext & context, HttpResponse & response);
	bool		isMethodAllowed(const RequestContext & context, const std::string & method);

	bool		isDirectory(const std::string & path);
	std::string	handleDirectoryPath(RequestContext & context);

	std::string	generateAutoindex(const std::string & dirPath, const std::string & requestPath);
	std::string	formatFileSize(off_t size);
	std::string	escapeHtml(const std::string & str);

	bool		validateBodySize(const Client & client, HttpResponse & response);
	std::string	getUploadPath(const RequestContext & context);
	std::string	generateAllowedMethodsHeader(const RequestContext & context);
	void		handlePostRequest(int fd, RequestContext & context, Client & client);
	void		handleDeleteRequest(int fd, RequestContext & context);

public:

	WebServer();
	~WebServer();
	WebServer(const WebServer & that);
	WebServer(const Config & config);
	WebServer&	operator=(const WebServer & that);
	
	void		run();
	void		stop();
};

#endif
