#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "webserv.hpp"

// TODO (#46 follow-up): replace this struct with an OCF-compliant Request
// class, as required by the subject. Kept as a struct for now to restore the
// parser (#9/#10/#42/#44) to a building, tested state without reshaping the
// integration contract before syncing with the reception loop.
struct t_request_line
{
	std::string	method;
	std::string	path;
	std::string	query;
	std::string	version;
};

int	parse_request_line(const std::string &raw, t_request_line &out,
		size_t &consumed);

int	parse_headers(const std::string &buffer, size_t headers_start,
		std::map<std::string, std::string> &out_headers,
		size_t &out_body_start);

BodyLengthStatus	detect_body_length(
		const std::map<std::string, std::string> &headers,
		size_t &out_length);

RequestStatus	check_request_complete(const std::string &buffer,
		size_t body_start, BodyLengthStatus length_status,
		size_t content_length);

#endif
