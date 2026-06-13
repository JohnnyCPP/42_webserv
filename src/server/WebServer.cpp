#include "server/WebServer.hpp"
#include "log/log.hpp"
#include "constants.hpp"

WebServer::WebServer()
	: servers(),
	  pollFds(),
	  clients(),
	  pendingResponses(),
	  running(false)
{
}

WebServer::~WebServer()
{
}

WebServer::WebServer(const WebServer & that)
	: servers(that.servers),
	  pollFds(that.pollFds),
	  clients(that.clients),
	  pendingResponses(that.pendingResponses),
	  running(that.running)
{
}

WebServer::WebServer(const Config & config)
	: pollFds(),
	  clients(),
	  pendingResponses(),
	  running(false)
{
	const std::vector<ServerConfig> &	serverConfigs = config.getServers();
	size_t								i;
	size_t								j;

	i = 0;
	while (i < serverConfigs.size())
	{
		const std::vector<std::string> & addresses = serverConfigs[i].getListenAddresses();
		j = 0;
		while (j < addresses.size())
		{
			ServerConfig serverConfig = serverConfigs[i];
			std::vector<std::string> address;
			address.push_back(addresses[j]);
			serverConfig.setListenAddresses(address);
			Server server(serverConfig);
			server.setup();
			servers.push_back(server);
			++j;
		}
		++i;
	}
}

WebServer &	WebServer::operator=(const WebServer & that)
{
	if (this != &that)
	{
		servers = that.servers;
		pollFds = that.pollFds;
		clients = that.clients;
		pendingResponses = that.pendingResponses;
		running = that.running;
	}
	return (*this);
}

void	WebServer::addToPoll(int fd, short events)
{
	struct pollfd	pollfd;

	pollfd.fd = fd;
	pollfd.events = events;
	pollfd.revents = 0;
	pollFds.push_back(pollfd);
}

void	WebServer::removeFromPoll(int fd)
{
	size_t	i;

	i = 0;
	while (i < pollFds.size())
	{
		if (pollFds[i].fd == fd)
		{
			pollFds.erase(pollFds.begin() + i);
			break;
		}
		++i;
	}
}

void	WebServer::getListeningSockets()
{
	std::ostringstream	stream;
	std::vector<int>	listenFds;
	int					listenFd;
	size_t				i;

	pollFds.clear();
	i = 0;
	while (i < servers.size())
	{
		listenFd = servers[i].getListenFd();
		if (listenFd != -1)
			addToPoll(listenFd, POLLIN);
		++i;
	}
	stream << "webserv is monitoring " << pollFds.size() << " listening sockets";
}

/**
 * There is data to read.
 */
void	WebServer::handlePollIn(struct pollfd current)
{
	std::ostringstream	stream;
	size_t				i;
	int					clientFd;

	stream << "webserv detected a POLLIN event on socket " << current.fd;
	log(stream.str());
	i = 0;
	while (i < servers.size())
	{
		if (servers[i].getListenFd() == current.fd)
		{
			clientFd = servers[i].acceptConnection();
			if (clientFd != -1)
			{
				clients.insert(std::make_pair(clientFd, Client(clientFd)));
				addToPoll(clientFd, POLLIN);
				clientToServer[clientFd] = &servers[i];
				stream.str("");
				stream.clear();
				stream << "webserv added a new client socket " << clientFd << " to poll()";
				log(stream.str());
			}
			return;
		}
		++i;
	}
	handleClientRead(current.fd);
}

/**
 * Writing is now possible, though a write larger than the 
 * available space in a socket or pipe will still block
 * unless O_NONBLOCK is set.
 */
void	WebServer::handlePollOut(struct pollfd current)
{
	std::map<int, std::string>::iterator	it;
	std::map<int, Client>::iterator			clientIt;
	std::ostringstream						stream;
	const char *							data;
	ssize_t									bytesSent;
	size_t									remaining;
	bool									keepAlive;
	bool									removeAfterSend;

	stream << "webserv detected a POLLOUT event on socket " << current.fd;
	log(stream.str());
	it = pendingResponses.find(current.fd);
	if (it == pendingResponses.end())
		return;
	data = it->second.c_str();
	remaining = it->second.size();
	bytesSent = send(current.fd, data, remaining, 0);
	if (bytesSent == -1)
		return;
	stream.str("");
	stream.clear();
	stream << "webserv sent " << bytesSent << " bytes";
	log(stream.str());
	if (bytesSent == static_cast<ssize_t>(remaining))
	{
		pendingResponses.erase(it);
		clientIt = clients.find(current.fd);
		if (clientIt != clients.end())
		{
			keepAlive = false; // TODO: implement keep-alive
			removeAfterSend = !keepAlive;
			if (removeAfterSend)
				removeClient(current.fd);
			else
			{
				clientIt->second.resetForNextRequest();
				modifyPollEvents(current.fd, POLLIN);
			}
		}
		else
			removeClient(current.fd);
	}
	else
		it->second = it->second.substr(bytesSent);
}

/**
 * POLLERR: An error condition is triggered. 
 *          This bit is also set for a file descriptor referring to 
 *          the write end of a pipe when the read end has been closed.
 *
 * POLLHUP: The channel is hang up. Note that when reading from a channel 
 *          such as a pipe or a stream socket, this event merely indicates 
 *          that the peer closed its end of the channel.
 *
 * POLLNVAL: Invalid request.
 */
void	WebServer::handlePollErr(struct pollfd current)
{
	std::ostringstream	stream;

	stream << "webserv detected a POLLERR, POLLHUP, or POLLNVAL event on socket " << current.fd;
	logError(stream.str());
	close(current.fd);
	removeFromPoll(current.fd);
}

/**
 * Explicitly avoiding determining the server behavior with errno.
 *
 * The purpose of that is to enforce proper non-blocking I/O design 
 * and separation of concerns.
 *
 * Checking errno, after recv() returns -1, makes its behavior 
 * dependent on OS-specific error codes.
 *
 * Different UNIX-like systems may return different error codes 
 * for the same situation.
 *
 * "Checking the value of errno to adjust the server behaviour 
 * is strictly forbidden after performing a read or write operation."
 *
 * If poll() returned POLLIN, recv() should not return -1 unless:
 *
 * - Connection was reset
 * - Or webserv read all data
 */
void	WebServer::handleClientRead(int fd)
{
	std::map<int, Server*>::iterator	serverIt;
	std::map<int, Client>::iterator		it;
	std::ostringstream					stream;
	ssize_t								bytesRead;
	size_t								maxSize;
	char								buffer[WebServ::RECV_BUFFER_SIZE];
	bool								keepReading;

	it = clients.find(fd);
	if (it == clients.end())
		return;
	serverIt = clientToServer.find(fd);
	if (serverIt != clientToServer.end())
	{
		maxSize = serverIt->second->getConfig().getClientMaxBodySize();
		it->second.setMaxBodySize(maxSize);
	}
	stream << "webserv is reading a client request on socket " << fd;
	log(stream.str());
	keepReading = true;
	while (keepReading)
	{
		bytesRead = recv(fd, buffer, sizeof(buffer), 0);
		if (bytesRead > 0)
		{
			stream.str("");
			stream.clear();
			stream << "webserv read " << bytesRead << " bytes from client socket " << fd;
			log(stream.str());
			it->second.appendToBuffer(std::string(buffer, bytesRead));
			it->second.parseRequest();
			if (it->second.isRequestComplete() || it->second.hasError())
				keepReading = false;
		}
		else if (bytesRead == 0)
		{
			removeClient(fd);
			return;
		}
		else
			keepReading = false;
	}
	if (it->second.isRequestComplete())
		processClientRequest(fd);
	else if (it->second.hasError())
	{
		stream.str("");
		stream.clear();
		stream << "webserv detected an error with client socket " << fd;
		logError(stream.str());
		removeClient(fd);
	}
}

/**
 * This function determines the appropiate response for a client socket 
 * after its HTTP request has been fully received.
 */
void	WebServer::processClientRequest(int fd)
{
	std::map<int, Client>::iterator	clientIt;
	std::ostringstream				stream;
	RequestContext					context;
	HttpResponse					response;
	std::string						allowedHeader;
	std::string						indexPath;
	std::string						autoindexHTML;
	std::string						requestURI;
	struct stat						statbuf;
	Client *						client;
	bool							autoindex;
	bool							hasIndexFile;

	clientIt = clients.find(fd);
	if (clientIt == clients.end())
		return;
	buildRequestContext(fd, context);
	client = &clientIt->second;
	stream << client->getMethod() << " " << client->getPath() << " " << client->getVersion();
	log(stream.str());
	if (!isMethodAllowed(context, clientIt->second.getMethod()))
	{
		logError("method not allowed");
		allowedHeader = generateAllowedMethodsHeader(context);
		response = HttpResponse::methodNotAllowed(allowedHeader);
		queueError(fd, response, context);
		return;
	}
	if (context.hasRedirect())
	{
		handleRedirect(context, response);
		log(std::string("redirecting to ") + response.getHeaders().find("Location")->second);
		pendingResponses[fd] = response.toString();
		modifyPollEvents(fd, POLLOUT);
		return;
	}
	resolveFilesystemPath(context);
	if (client->getMethod() == "POST")
	{
		handlePostRequest(fd, context, *client);
		return;
	}
	if (client->getMethod() == "DELETE")
	{
		handleDeleteRequest(fd, context);
		return;
	}
	if (stat(context.getResolvedPath().c_str(), &statbuf) != 0)
	{
		log(std::string("resource ") + context.getResolvedPath() + std::string(" was not found"));
		response = HttpResponse::notFound();
		queueError(fd, response, context);
		return;
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		log(std::string("resource ") + context.getResolvedPath() + std::string(" is a directory"));
		autoindex = false;
		if (context.getMatchedLocation() != NULL)
			autoindex = context.getMatchedLocation()->getAutoindex();
		if (autoindex)
		{
			log("autoindex is enabled");
			indexPath = context.getResolvedPath();
			if (indexPath[indexPath.length() - 1] != '/')
				indexPath += '/';
			if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getIndex().empty())
				indexPath += context.getMatchedLocation()->getIndex();
			else
				indexPath += context.getTargetServer()->getIndex();
			hasIndexFile = (stat(indexPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
			if (!hasIndexFile)
			{
				requestURI = context.getRequestPath();
				if (requestURI.empty() || requestURI[requestURI.length() - 1] != '/')
					requestURI += '/';
				autoindexHTML = generateAutoindex(context.getResolvedPath(), requestURI);
				response.setStatus(200);
				response.setHeader("Content-Type", "text/html");
				response.setBody(autoindexHTML);
				pendingResponses[fd] = response.toString();
				modifyPollEvents(fd, POLLOUT);
				log(std::string("generated autoindex for ") + context.getResolvedPath());
				return;
			}
		}
		else
			log("autoindex is disabled");
		context.setResolvedPath(handleDirectoryPath(context));
		if (stat(context.getResolvedPath().c_str(), &statbuf) != 0)
		{
			log(std::string("directory ") + context.getResolvedPath() + std::string(" has no index file and autoindex is off"));
			response = HttpResponse::forbidden();
			queueError(fd, response, context);
			return;
		}
	}
	if (!S_ISREG(statbuf.st_mode))
	{
		log(std::string("resource ") + context.getResolvedPath() + std::string(" is not a regular file. It may be a device, socket, symlink, or other"));
		response = HttpResponse::forbidden();
		queueError(fd, response, context);
		return;
	}
	response.setBodyFromFile(context.getResolvedPath());
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("webserv will serve a file located at ") + context.getResolvedPath());
}

void	WebServer::removeClient(int fd)
{
	std::map<int, std::string>::iterator	pendingIt;
	std::map<int, Client>::iterator			clientIt;
	std::map<int, Server*>::iterator		serverIt;
	std::ostringstream						stream;

	close(fd);
	clientIt = clients.find(fd);
	if (clientIt != clients.end())
		clients.erase(clientIt);
	pendingIt = pendingResponses.find(fd);
	if (pendingIt != pendingResponses.end())
		pendingResponses.erase(pendingIt);
	serverIt = clientToServer.find(fd);
	if (serverIt != clientToServer.end())
		clientToServer.erase(serverIt);
	clientsToRemove.push_back(fd);
	stream << "webserv marked client socket " << fd << " for removal";
	log(stream.str());
}

void	WebServer::cleanupRemovedClients()
{
	std::ostringstream	stream;
	size_t				clientsCount;
	size_t				pollCount;
	size_t				i;
	size_t				j;

	log("webserv is looking for removed clients");
	clientsCount = clientsToRemove.size();
	pollCount = pollFds.size();
	i = 0;
	while (i < clientsCount)
	{
		j = 0;
		while (j < pollCount)
		{
			if (pollFds[j].fd == clientsToRemove[i])
			{
				stream << "webserv removed client socket " << pollFds[j].fd;
				log(stream.str());
				pollFds.erase(pollFds.begin() + j);
				pollCount = pollCount - 1;
				break;
			}
			++j;
		}
		++i;
	}
	clientsToRemove.clear();
}

void	WebServer::modifyPollEvents(int fd, short events)
{
	size_t	i;

	i = 0;
	while (i < pollFds.size())
	{
		if (pollFds[i].fd == fd)
		{
			pollFds[i].events = events;
			break;
		}
		++i;
	}
}

/**
 * Gathers initial information about the request and stores it in context.
 */
void	WebServer::buildRequestContext(int clientFd, RequestContext & context)
{
	std::map<int, Server*>::iterator	serverIt;
	std::map<int, Client>::iterator		clientIt;
	const LocationConfig *				matchedLocation;
	std::string							requestPath;
	size_t								queryPos;

	clientIt = clients.find(clientFd);
	if (clientIt == clients.end())
		return;
	serverIt = clientToServer.find(clientFd);
	if (serverIt == clientToServer.end())
		return;
	requestPath = clientIt->second.getPath();
	queryPos = requestPath.find('?');
	if (queryPos != std::string::npos)
		requestPath = requestPath.substr(0, queryPos);
	context.setRequestPath(requestPath);
	context.setTargetServer(&(serverIt->second->getConfig()));
	matchedLocation = context.getTargetServer()->matchLocation(requestPath);
	context.setMatchedLocation(matchedLocation);
}

/**
 * Converts the URL path to a filesystem path.
 *
 * Example:
 *
 * If server has a location path "/public" whose root is "./www/public" 
 * and the request is "/public/subdir/file.txt", then:
 *
 * 1: locationPath = "/public"
 * 2: remainingPath = requestPath without locationPath = "/subdir/file.txt"
 * 3: root = "./www/public"
 * 4: resolved = root + remainingPath = "./www/public/subdir/file.txt"
 */
void	WebServer::resolveFilesystemPath(RequestContext & context)
{
	std::string	remainingPath;
	std::string	locationPath;
	std::string	requestPath;
	std::string	resolved;
	std::string	root;

	requestPath = context.getRequestPath();
	root = context.getTargetServer()->getRoot();
	if (context.hasCustomRoot())
		root = context.getMatchedLocation()->getRoot();
	if (context.getMatchedLocation() != NULL)
	{
		locationPath = context.getMatchedLocation()->getPath();
		if (requestPath.find(locationPath) == 0)
		{
			remainingPath = requestPath.substr(locationPath.length());
			if (remainingPath.empty() || remainingPath[0] != '/')
				remainingPath = "/" + remainingPath;
		}
		else
			remainingPath = requestPath;
	}
	else
		remainingPath = requestPath;
	if (root[root.length() - 1] == '/')
		resolved = root;
	else
		resolved = root + "/";
	if (!remainingPath.empty() && remainingPath[0] == '/')
		remainingPath = remainingPath.substr(1);
	resolved += remainingPath;
	context.setResolvedPath(resolved);
}

/**
 * If the location has a return directive, creates a redirect response.
 *
 * Example:
 *
 * For a given location block:
 *
 * location /old-stuff {
 *     return 301 /public;
 * }
 *
 * 1. Detects hasRedirect() = true
 * 2. Reads redirect target = /public
 * 3. Detects 301 in the string
 * 4. Creates HTTP response: 301 Moved Permanently with Location: /public
 */
void	WebServer::handleRedirect(const RequestContext & context, HttpResponse & response)
{
	std::string	redirectTarget;
	int			statusCode;

	if (!context.hasRedirect())
		return;
	redirectTarget = context.getMatchedLocation()->getRedirect();
	statusCode = 301;
	if (redirectTarget.find("302") == 0)
	{
		statusCode = 302;
		if (redirectTarget.length() > 4)
			redirectTarget = redirectTarget.substr(4);
		else
			redirectTarget = "/";
	}
	else if (redirectTarget.find("301") == 0)
	{
		statusCode = 301;
		if (redirectTarget.length() > 4)
			redirectTarget = redirectTarget.substr(4);
		else
			redirectTarget = "/";
	}
	if (statusCode == 301)
		response = HttpResponse::movedPermanently(redirectTarget);
	else
		response = HttpResponse::found(redirectTarget);
}

/**
 * Checks if the HTTP method is allowed for this location.
 *
 * If the context lacks a matched location, all methods are allowed.
 *
 * This is intentional, because to forbid methods, the configuration 
 * file allows the administrator to explicitly forbid all methods 
 * for a location if "allow_methods" directive is missing.
 *
 * Example:
 *
 * Given the following location blocks:
 *
 * location /public {
 *     allow_methods GET;
 * }
 * 
 * location /api {
 *     allow_methods GET POST DELETE;
 * }
 * 
 * location / {
 * }
 *
 * A request whose method is POST is allowed for path "/api" 
 * but not allowed for path "/public".
 *
 * All methods are forbidden for path "/".
 */
bool	WebServer::isMethodAllowed(const RequestContext & context, const std::string & method)
{
	const std::vector<std::string> *	allowedMethods;
	size_t								i;

	if (context.getMatchedLocation() == NULL)
		return (true);
	allowedMethods = &(context.getMatchedLocation()->getAllowedMethods());
	if (allowedMethods->empty())
		return (false);
	i = 0;
	while (i < allowedMethods->size())
	{
		if ((*allowedMethods)[i] == method)
			return (true);
		++i;
	}
	return (false);
}

bool	WebServer::isDirectory(const std::string & path)
{
	struct stat	statbuf;
	int			result;

	result = stat(path.c_str(), &statbuf);
	if (result != 0)
		return (false);
	return (S_ISDIR(statbuf.st_mode));
}

/**
 * When the resolved path is a directory, determines what file to serve.
 *
 * Priority chain:
 *
 * 1. Location's index directive (if specified)
 * 2. Server's index directive (if specified)
 * 3. Default "index.html"
 */
std::string	WebServer::handleDirectoryPath(RequestContext & context)
{
	std::string	resolvedPath;
	std::string	indexFile;
	std::string	indexPath;

	resolvedPath = context.getResolvedPath();
	if (resolvedPath[resolvedPath.length() - 1] != '/')
		resolvedPath += '/';
	if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getIndex().empty())
		indexFile = context.getMatchedLocation()->getIndex();
	else
		indexFile = context.getTargetServer()->getIndex();
	if (indexFile.empty())
		indexFile = WebServ::DEFAULT_INDEX;
	indexPath = resolvedPath + indexFile;
	return (indexPath);
}

std::string	WebServer::generateAutoindex(const std::string & dirPath, const std::string & requestPath)
{
	std::vector<std::string>	items;
	struct dirent *				entry;
	std::string					requestPathWithSlash;
	std::string					displayPath;
	std::string					fullPath;
	std::string					result;
	struct stat					entryStat;
	size_t						i;
	DIR *						dir;
	char						buffer[64];

	dir = opendir(dirPath.c_str());
	if (dir == NULL)
		return ("");
	result = "<html>\n<head>\n<title>Index of ";
	result += escapeHtml(requestPath);
	result += "</title>\n</head>\n<body>\n<h1>Index of ";
	result += escapeHtml(requestPath);
	result += "</h1>\n<hr>\n<pre>\n";
	requestPathWithSlash = requestPath;
	if (requestPathWithSlash.empty() || requestPathWithSlash[requestPathWithSlash.length() - 1] != '/')
		requestPathWithSlash += '/';
	if (requestPath != "/")
		result += "<a href=\"../\">../</a>\n";
	while (true)
	{
		entry = readdir(dir);
		if (entry == NULL)
			break;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '\0')
			continue;
		if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0')
			continue;
		items.push_back(std::string(entry->d_name));
	}
	closedir(dir);
	i = 0;
	while (i < items.size())
	{
		fullPath = dirPath;
		if (fullPath[fullPath.length() - 1] != '/')
			fullPath += '/';
		fullPath += items[i];
		if (stat(fullPath.c_str(), &entryStat) == 0)
		{
			result += "<a href=\"";
			result += escapeHtml(items[i]);
			if (S_ISDIR(entryStat.st_mode))
				result += "/";
			result += "\">";
			result += escapeHtml(items[i]);
			if (S_ISDIR(entryStat.st_mode))
				result += "/";
			result += "</a>";
			if (S_ISDIR(entryStat.st_mode))
				result += "                    -";
			else
			{
				while (result.length() < 50)
					result += " ";
				result += formatFileSize(entryStat.st_size);
			}
			while (result.length() < 70)
				result += " ";
			strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", localtime(&entryStat.st_mtime));
			result += buffer;
			result += "\n";
		}
		++i;
	}
	result += "</pre>\n<hr>\n</body>\n</html>\n";
	return (result);
}

std::string	WebServer::formatFileSize(off_t size)
{
	std::ostringstream	stream;
	char				buffer[64];

	if (size < 1024)
		stream << size << " B";
	else if (size < 1024 * 1024)
	{
		std::snprintf(buffer, sizeof(buffer), "%.1f KB", size / 1024.0);
		stream << buffer;
	}
	else if (size < 1024 * 1024 * 1024)
	{
		std::snprintf(buffer, sizeof(buffer), "%.1f MB", size / (1024.0 * 1024.0));
		stream << buffer;
	}
	else
	{
		std::snprintf(buffer, sizeof(buffer), "%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
		stream << buffer;
	}
	return (stream.str());
}

std::string	WebServer::escapeHtml(const std::string & str)
{
	std::string	result;
	size_t		i;
	char		c;

	i = 0;
	while (i < str.length())
	{
		c = str[i];
		if (c == '&')
			result += "&amp;";
		else if (c == '<')
			result += "&lt;";
		else if (c == '>')
			result += "&gt;";
		else if (c == '"')
			result += "&quot;";
		else
			result += c;
		++i;
	}
	return (result);
}

bool	WebServer::validateBodySize(const Client & client, HttpResponse & response)
{
	bool	result;

	result = true;
	if (client.isBodySizeExceeded())
	{
		response = HttpResponse::payloadTooLarge();
		result = false;
	}
	return (result);
}

std::string	WebServer::getUploadPath(const RequestContext & context)
{
	std::ostringstream	stream;
	std::string			uploadPath;
	std::string			filename;
	size_t				lastSlash;

	if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getUploadStore().empty())
		uploadPath = context.getMatchedLocation()->getUploadStore();
	else
		uploadPath = WebServ::DEFAULT_UPLOADS;
	if (uploadPath[uploadPath.length() - 1] != '/')
		uploadPath += '/';
	filename = context.getRequestPath();
	lastSlash = filename.rfind('/');
	if (lastSlash != std::string::npos)
		filename = filename.substr(lastSlash + 1);
	if (filename.empty())
	{
		stream << time(NULL);
		filename = stream.str();
	}
	uploadPath += filename;
	return (uploadPath);
}

std::string	WebServer::generateAllowedMethodsHeader(const RequestContext & context)
{
	const std::vector<std::string> *	allowedMethods;
	std::string							result;
	size_t								i;

	if (context.getMatchedLocation() == NULL)
		return ("GET, POST, DELETE");
	allowedMethods = &(context.getMatchedLocation()->getAllowedMethods());
	i = 0;
	while (i < allowedMethods->size())
	{
		if (i > 0)
			result += ", ";
		result += (*allowedMethods)[i];
		++i;
	}
	return (result);
}

/**
 * Replaces an error response's body with the server's configured error_page,
 * when one exists for that status code and the file can be read. The status
 * code is preserved; on any failure the default body set by the factory stays.
 */
void	WebServer::applyErrorPage(HttpResponse & response, const RequestContext & context)
{
	std::map<int, std::string>::const_iterator	it;
	std::stringstream							buffer;
	std::ifstream								file;
	std::string									path;

	if (context.getTargetServer() == NULL)
		return;
	const std::map<int, std::string> &	pages = context.getTargetServer()->getErrorPages();
	it = pages.find(response.getStatusCode());
	if (it == pages.end())
		return;
	path = context.getTargetServer()->getRoot();
	if (!path.empty() && path[path.length() - 1] == '/')
		path.erase(path.length() - 1);
	if (!it->second.empty() && it->second[0] != '/')
		path += '/';
	path += it->second;
	file.open(path.c_str());
	if (!file.is_open())
	{
		logError(std::string("error_page not readable: ") + path);
		return;
	}
	buffer << file.rdbuf();
	file.close();
	response.setBody(buffer.str());
	response.setContentType(".html");
}

void	WebServer::queueError(int fd, HttpResponse & response, const RequestContext & context)
{
	applyErrorPage(response, context);
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
}

/**
 * The flag std::ios::out opens a file for writing.
 *
 * The flag std::ios::binary opens a file in binary mode:
 *
 * When a file is opened in C++, there are two modes 
 * for handling newline characters:
 *
 * - text mode (default): On Windows, \n gets converted to \r\n on write. 
 *                        On Unix-like systems (Linux, macOS), 
 *                        no conversion happens.
 *
 * - binary mode: No newline conversion. Bytes are written as given.
 */
void	WebServer::handlePostRequest(int fd, RequestContext & context, Client & client)
{
	std::ofstream	file;
	HttpResponse	response;
	std::string		uploadPath;

	if (!validateBodySize(client, response))
	{
		queueError(fd, response, context);
		return;
	}
	if (context.getMatchedLocation() == NULL || context.getMatchedLocation()->getUploadStore().empty())
	{
		response = HttpResponse::notImplemented();
		queueError(fd, response, context);
		return;
	}
	uploadPath = getUploadPath(context);
	file.open(uploadPath.c_str(), std::ios::out | std::ios::binary);
	if (!file.is_open())
	{
		response = HttpResponse::internalServerError();
		queueError(fd, response, context);
		return;
	}
	file.write(client.getBody().c_str(), client.getBody().size());
	file.close();
	response = HttpResponse::created(uploadPath);
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("uploaded file to ") + uploadPath);
}

void	WebServer::handleDeleteRequest(int fd, RequestContext & context)
{
	HttpResponse	response;
	std::string		targetPath;
	struct stat		statbuf;

	if (context.getMatchedLocation() != NULL && !context.getMatchedLocation()->getUploadStore().empty())
		targetPath = getUploadPath(context);
	else
		targetPath = context.getResolvedPath();
	if (stat(targetPath.c_str(), &statbuf) != 0)
	{
		response = HttpResponse::notFound();
		queueError(fd, response, context);
		return;
	}
	if (access(targetPath.c_str(), W_OK) != 0)
	{
		response = HttpResponse::forbidden();
		queueError(fd, response, context);
		return;
	}
	if (unlink(targetPath.c_str()) != 0)
	{
		response = HttpResponse::internalServerError();
		queueError(fd, response, context);
		return;
	}
	response = HttpResponse::noContent();
	pendingResponses[fd] = response.toString();
	modifyPollEvents(fd, POLLOUT);
	log(std::string("deleted file ") + targetPath);
}

/**
 * A socket that has both data to read AND can write 
 * revents could be: POLLIN | POLLOUT (both bits set)
 *
 * If the control structure is written as:
 * 
 * if (POLLIN) {}
 * else if (POLLOUT) {}
 *
 * It's wrong because the server will handle POLLIN only
 */
void	WebServer::run()
{
	extern volatile	sig_atomic_t	g_running;
	size_t							i;
	int								readyFds;

	getListeningSockets();
	if (pollFds.empty())
	{
		logError("webserv lacks listening sockets, exiting...");
		return;
	}
	running = true;
	log("webserv is entering to the event loop");
	while (running && g_running)
	{
		cleanupRemovedClients();
		log("webserv is executing poll()");
		readyFds = poll(&pollFds[0], pollFds.size(), -1);
		if (readyFds == -1)
		{
			if (errno == EINTR && !g_running)
				break;
			logError(std::string("poll() failed: ") + strerror(errno));
			break;
		}
		i = 0;
		while (i < pollFds.size())
		{
			if (pollFds[i].revents & POLLIN)
				handlePollIn(pollFds[i]);
			if (pollFds[i].revents & POLLOUT)
				handlePollOut(pollFds[i]);
			if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				handlePollErr(pollFds[i]);
				continue;
			}
			++i;
		}
	}
	log("webserv is shutting down");
	i = 0;
	while (i < pollFds.size())
	{
		close(pollFds[i].fd);
		++i;
	}
	pollFds.clear();
	clients.clear();
	clientToServer.clear();
	pendingResponses.clear();
}

void	WebServer::stop()
{
	running = false;
}
