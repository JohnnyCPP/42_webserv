#include "session/SessionManager.hpp"
#include "log/log.hpp"

SessionManager::SessionManager()
	: sessions(),
	  sessionTimeout(3600)
{
}

SessionManager::~SessionManager()
{
}

SessionManager::SessionManager(const SessionManager & that)
	: sessions(that.sessions),
	  sessionTimeout(that.sessionTimeout)
{
}

SessionManager & SessionManager::operator=(const SessionManager & that)
{
	if (this != &that)
	{
		sessions = that.sessions;
		sessionTimeout = that.sessionTimeout;
	}
	return (*this);
}

std::string	SessionManager::generateSessionId()
{
	std::ostringstream	stream;
	char				buffer[33];
	int					fd;
	ssize_t				bytesRead;
	size_t				i;
	unsigned char		randomData[16];
	static const char	hexChars[] = "0123456789abcdef";

	fd = open("/dev/urandom", O_RDONLY);
	if (fd != -1)
	{
		bytesRead = read(fd, randomData, sizeof(randomData));
		close(fd);
		if (bytesRead == static_cast<ssize_t>(sizeof(randomData)))
		{
			i = 0;
			while (i < sizeof(randomData))
			{
				buffer[i * 2] = hexChars[randomData[i] >> 4];
				buffer[i * 2 + 1] = hexChars[randomData[i] & 0x0f];
				++i;
			}
			buffer[32] = '\0';
			stream << buffer;
			return (stream.str());
		}
	}
	stream << time(NULL) << getpid() << rand();
	return (stream.str());
}

void	SessionManager::setSessionTimeout(time_t seconds)
{
	sessionTimeout = seconds;
}

std::string	SessionManager::createSession()
{
	std::string	sessionId;
	Session		newSession;

	cleanupExpiredSessions();
	sessionId = generateSessionId();
	newSession = Session(sessionId);
	sessions[sessionId] = newSession;
	log(std::string("created session: ") + sessionId);
	return (sessionId);
}

Session *	SessionManager::getSession(const std::string & sessionId)
{
	std::map<std::string, Session>::iterator	it;

	cleanupExpiredSessions();
	it = sessions.find(sessionId);
	if (it == sessions.end())
		return (NULL);
	if (it->second.isExpired(sessionTimeout))
	{
		sessions.erase(it);
		return (NULL);
	}
	it->second.touch();
	return (&(it->second));
}

void	SessionManager::destroySession(const std::string & sessionId)
{
	std::map<std::string, Session>::iterator	it;

	it = sessions.find(sessionId);
	if (it != sessions.end())
	{
		log(std::string("destroyed session: ") + sessionId);
		sessions.erase(it);
	}
}

void	SessionManager::addSessionData(const std::string & sessionId, const std::string & key, const std::string & value)
{
	Session *	session;

	session = getSession(sessionId);
	if (session != NULL)
		session->data[key] = value;
}

std::string	SessionManager::getSessionData(const std::string & sessionId, const std::string & key)
{
	std::map<std::string, std::string>::iterator	it;
	Session *										session;

	session = getSession(sessionId);
	if (session == NULL)
		return ("");
	it = session->data.find(key);
	if (it == session->data.end())
		return ("");
	return (it->second);
}

std::string	SessionManager::extractSessionIdFromCookie(const std::string & cookieHeader)
{
	std::string	sessionId;
	size_t		pos;
	size_t		end;
	size_t		start;

	pos = cookieHeader.find("session_id=");
	if (pos == std::string::npos)
		return ("");
	start = pos + 11;
	end = cookieHeader.find(';', start);
	if (end == std::string::npos)
		end = cookieHeader.length();
	sessionId = cookieHeader.substr(start, end - start);
	return (sessionId);
}

void	SessionManager::cleanupExpiredSessions()
{
	std::map<std::string, Session>::iterator	it;
	std::map<std::string, Session>::iterator	next;
	std::ostringstream							stream;
	size_t										count;

	count = 0;
	it = sessions.begin();
	while (it != sessions.end())
	{
		next = it;
		++next;
		if (it->second.isExpired(sessionTimeout))
		{
			sessions.erase(it);
			++count;
		}
		it = next;
	}
	if (count > 0)
	{
		stream << "cleaned up " << count << " expired sessions";
		log(stream.str());
	}
}

void	SessionManager::cleanup()
{
	sessions.clear();
}
