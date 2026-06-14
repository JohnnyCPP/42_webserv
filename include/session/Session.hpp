#ifndef SESSION_HPP
# define SESSION_HPP

# include "webserv.hpp"

class Session
{
public:

	std::string							id;
	std::map<std::string, std::string>	data;
	time_t								createdAt;
	time_t								lastAccessed;

	Session();
	~Session();
	Session(const std::string & sessionId);
	Session(const Session & that);
	Session & operator=(const Session & that);

	bool	isExpired(time_t timeout) const;
	void	touch();
};

#endif
