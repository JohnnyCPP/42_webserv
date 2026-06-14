#include "session/Session.hpp"
#include "log/log.hpp"

Session::Session()
	: id(""),
	  data(),
	  createdAt(time(NULL)),
	  lastAccessed(time(NULL))
{
}

Session::Session(const std::string & sessionId)
	: id(sessionId),
	  data(),
	  createdAt(time(NULL)),
	  lastAccessed(time(NULL))
{
}

Session::~Session()
{
}

Session::Session(const Session & that)
	: id(that.id),
	  data(that.data),
	  createdAt(that.createdAt),
	  lastAccessed(that.lastAccessed)
{
}

Session & Session::operator=(const Session & that)
{
	if (this != &that)
	{
		id = that.id;
		data = that.data;
		createdAt = that.createdAt;
		lastAccessed = that.lastAccessed;
	}
	return (*this);
}

bool	Session::isExpired(time_t timeout) const
{
	time_t	now;
	time_t	idleTime;

	now = time(NULL);
	idleTime = now - lastAccessed;
	return (idleTime > timeout);
}

void	Session::touch()
{
	lastAccessed = time(NULL);
}
