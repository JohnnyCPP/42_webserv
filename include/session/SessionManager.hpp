#ifndef SESSION_MANAGER_HPP
# define SESSION_MANAGER_HPP

# include "webserv.hpp"
# include "session/Session.hpp"

class SessionManager
{
private:

	std::map<std::string, Session>	sessions;
	time_t							sessionTimeout;

	std::string	generateSessionId();
	void		cleanupExpiredSessions();

public:

	SessionManager();
	~SessionManager();
	SessionManager(const SessionManager & that);
	SessionManager & operator=(const SessionManager & that);

	void		setSessionTimeout(time_t seconds);
	std::string	createSession();
	Session *	getSession(const std::string & sessionId);
	void		destroySession(const std::string & sessionId);
	void		addSessionData(const std::string & sessionId, const std::string & key, const std::string & value);
	std::string	getSessionData(const std::string & sessionId, const std::string & key);
	std::string	extractSessionIdFromCookie(const std::string & cookieHeader);
	void		cleanup();
};

#endif
