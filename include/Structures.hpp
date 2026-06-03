#ifndef WS_STRUCTURES_HPP
# define WS_STRUCTURES_HPP

struct t_request_line
{
	std::string	method;	 
	std::string	path;	 
	std::string	query;	
	std::string	version;
};

#endif
