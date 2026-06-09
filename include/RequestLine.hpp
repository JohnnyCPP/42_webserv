#ifndef REQUEST_LINE_HPP
# define REQUEST_LINE_HPP

# include "webserv.hpp"

/**
 * Parsed HTTP request line: method, path, query string, and version.
 *
 * Replaces the former t_request_line struct with an OCF-compliant class, as
 * required by the subject (no structs in C++) and noted in commit de00544.
 */
class RequestLine
{
	private:

		std::string	method;
		std::string	path;
		std::string	query;
		std::string	version;

	public:

		RequestLine();
		RequestLine(RequestLine const & that);
		~RequestLine();
		RequestLine & operator=(RequestLine const & that);

		void	setMethod(std::string const & value);
		void	setPath(std::string const & value);
		void	setQuery(std::string const & value);
		void	setVersion(std::string const & value);

		std::string const &	getMethod() const;
		std::string const &	getPath() const;
		std::string const &	getQuery() const;
		std::string const &	getVersion() const;
};

#endif
