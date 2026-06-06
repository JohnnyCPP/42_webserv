#ifndef STRUCTURES_HPP
# define STRUCTURES_HPP

// DON'T USE STRUCTURES IN C++
//
// USE CLASSES INSTEAD, AS PER SUBJECT REQUIREMENT

// TODO: replace this structure by a C++ OCF compliant class

struct t_request_line
{
	std::string	method;	 
	std::string	path;	 
	std::string	query;	
	std::string	version;
};

#endif
