#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "webserv.hpp"
# include "RequestLine.hpp"

int	parse_request_line(const std::string &raw, RequestLine &out,
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
