#ifndef PROTOTYPES_HPP
# define PROTOTYPES_HPP

# include "webserv.hpp"

enum BodyLengthStatus
{
	BODY_NONE,
	BODY_LENGTH,
	BODY_CHUNKED,
	BODY_ERROR
};

enum RequestStatus
{
	REQ_INCOMPLETE,
	REQ_COMPLETE,
	REQ_ERROR
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
