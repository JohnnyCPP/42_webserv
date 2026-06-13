#ifndef CGI_HANDLER_HPP
# define CGI_HANDLER_HPP

# include "webserv.hpp"
# include "config/ServerConfig.hpp"
# include "config/LocationConfig.hpp"
# include "client/Client.hpp"
# include "http/HttpResponse.hpp"
# include "http/RequestContext.hpp"

class CgiHandler
{
private:

	pid_t					pid;
	int						pipeStdin;
	int						pipeStdout;
	int						clientFd;
	std::string				output;
	const ServerConfig *	serverConfig;
	bool					isActive;

	std::map<std::string, std::string>	buildEnv(const Client & client, const RequestContext & context, const std::string & scriptPath) const;
	std::string							extractScriptPath(const RequestContext & context) const;
	std::string							findInterpreter(const std::string & scriptPath) const;
	std::string							parseCgiOutput(const std::string & output, HttpResponse & response) const;
	void								cleanup();

public:

	CgiHandler();
	~CgiHandler();
	CgiHandler(const CgiHandler & that);
	CgiHandler & operator=(const CgiHandler & that);

	void	startExecution(int clientFd, const Client & client, const RequestContext & context, std::vector<struct pollfd> & pollFds, std::map<int, int> & clientToPipe, std::map<int, const ServerConfig *> & pipeToServer);
	int		handlePipeOutput(int pipeFd, std::map<int, std::string> & pendingResponses, std::vector<struct pollfd> & pollFds, std::map<int, int> & clientToPipe, std::map<int, const ServerConfig *> & pipeToServer);
	bool	isCgiRequest(const RequestContext & context) const;
	bool	hasActiveCgi() const;
};

#endif
