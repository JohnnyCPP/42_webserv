#include "request.hpp"
#include <cctype>
#include <limits>

static bool	method_is_well_formed(const std::string &method)
{
	std::string::size_type	i;

	if (method.empty())
		return (false);
	for (i = 0; i < method.size(); ++i)
	{
		if (method[i] < 'A' || method[i] > 'Z')
			return (false);
	}
	return (true);
}

static int	check_version(const std::string &version)
{
	if (version == WebServ::HTTP_VERSION || version == WebServ::HTTP_VERSION_LEGACY)
		return (WebServ::OK);
	if (version.size() == 8 && version.compare(0, 5, "HTTP/") == 0
		&& version[5] >= '0' && version[5] <= '9'
		&& version[6] == '.'
		&& version[7] >= '0' && version[7] <= '9')
		return (WebServ::VERSION_NOT_SUPPORTED);
	return (WebServ::BAD_REQUEST);
}

static int	assign_path(const std::string &target, t_request_line &out)
{
	std::string::size_type	q;

	q = target.find('?');
	if (q == std::string::npos)
	{
		out.path = target;
		out.query.clear();
	}
	else
	{
		out.path = target.substr(0, q);
		out.query = target.substr(q + 1);
	}
	if (out.path.empty() || out.path[0] != '/')
		return (WebServ::BAD_REQUEST);
	return (WebServ::OK);
}

int	parse_request_line(const std::string &raw, t_request_line &out,
		size_t &consumed)
{
	std::string::size_type	eol;
	std::string::size_type	sp1;
	std::string::size_type	sp2;
	std::string				line;
	std::string				target;
	int						status;

	consumed = 0;
	eol = raw.find(WebServ::CRLF);
	if (eol == std::string::npos)
		return (WebServ::REQUEST_INCOMPLETE);
	line = raw.substr(0, eol);

	sp1 = line.find(' ');
	if (sp1 == std::string::npos)
		return (WebServ::BAD_REQUEST);
	sp2 = line.find(' ', sp1 + 1);
	if (sp2 == std::string::npos)
		return (WebServ::BAD_REQUEST);

	out.method = line.substr(0, sp1);
	target = line.substr(sp1 + 1, sp2 - sp1 - 1);
	out.version = line.substr(sp2 + 1);

	if (out.version.find(' ') != std::string::npos)
		return (WebServ::BAD_REQUEST);
	if (!method_is_well_formed(out.method))
		return (WebServ::BAD_REQUEST);

	status = assign_path(target, out);
	if (status != WebServ::OK)
		return (status);
	status = check_version(out.version);
	if (status != WebServ::OK)
		return (status);

	consumed = eol + WebServ::CRLF.size();
	return (WebServ::OK);
}

static bool	is_token_char(char c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9'))
		return (true);
	switch (c)
	{
		case '!': case '#': case '$': case '%': case '&': case '\'':
		case '*': case '+': case '-': case '.': case '^': case '_':
		case '`': case '|': case '~':
			return (true);
	}
	return (false);
}

static void	lowercase(std::string &s)
{
	std::string::size_type	i;

	for (i = 0; i < s.size(); ++i)
		s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
}

static std::string	trim_ows(const std::string &s)
{
	std::string::size_type	start;
	std::string::size_type	end;

	start = 0;
	end = s.size();
	while (start < end && (s[start] == ' ' || s[start] == '\t'))
		++start;
	while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
		--end;
	return (s.substr(start, end - start));
}

static int	parse_header_line(const std::string &line,
		std::map<std::string, std::string> &out)
{
	std::string::size_type							colon;
	std::string										name;
	std::string										value;
	std::string::size_type							i;
	std::map<std::string, std::string>::iterator	it;

	if (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
		return (WebServ::BAD_REQUEST);
	colon = line.find(':');
	if (colon == std::string::npos)
		return (WebServ::BAD_REQUEST);
	name = line.substr(0, colon);
	value = trim_ows(line.substr(colon + 1));
	if (name.empty())
		return (WebServ::BAD_REQUEST);
	for (i = 0; i < name.size(); ++i)
		if (!is_token_char(name[i]))
			return (WebServ::BAD_REQUEST);
	lowercase(name);
	it = out.find(name);
	if (it != out.end())
	{
		if (name == "content-length" && it->second != value)
			return (WebServ::BAD_REQUEST);
		it->second = value;
	}
	else
		out[name] = value;
	return (WebServ::OK);
}

int	parse_headers(const std::string &buffer, size_t headers_start,
		std::map<std::string, std::string> &out_headers, size_t &out_body_start)
{
	std::string::size_type				marker;
	size_t								region_end;
	size_t								pos;
	std::string::size_type				line_end;
	std::string							line;
	std::map<std::string, std::string>	headers;
	int									status;

	marker = buffer.find("\r\n\r\n");
	if (marker == std::string::npos)
		return (WebServ::REQUEST_INCOMPLETE);
	region_end = marker + 2;
	pos = headers_start;
	while (pos < region_end)
	{
		line_end = buffer.find("\r\n", pos);
		line = buffer.substr(pos, line_end - pos);
		status = parse_header_line(line, headers);
		if (status != WebServ::OK)
			return (status);
		pos = line_end + 2;
	}
	out_headers = headers;
	out_body_start = marker + 4;
	return (WebServ::OK);
}

static BodyLengthStatus	parse_content_length(const std::string &value,
		size_t &out_length)
{
	std::string::size_type	i;
	size_t					result;
	size_t					digit;

	if (value.empty())
		return (BODY_ERROR);
	result = 0;
	for (i = 0; i < value.size(); ++i)
	{
		if (value[i] < '0' || value[i] > '9')
			return (BODY_ERROR);
		digit = static_cast<size_t>(value[i] - '0');
		if (result > (std::numeric_limits<size_t>::max() - digit) / 10)
			return (BODY_ERROR);
		result = result * 10 + digit;
	}
	out_length = result;
	return (BODY_LENGTH);
}

BodyLengthStatus	detect_body_length(
		const std::map<std::string, std::string> &headers, size_t &out_length)
{
	std::map<std::string, std::string>::const_iterator	it;

	it = headers.find("transfer-encoding");
	if (it != headers.end() && it->second.find("chunked") != std::string::npos)
		return (BODY_CHUNKED);
	it = headers.find("content-length");
	if (it == headers.end())
		return (BODY_NONE);
	return (parse_content_length(it->second, out_length));
}

RequestStatus	check_request_complete(const std::string &buffer,
		size_t body_start, BodyLengthStatus length_status, size_t content_length)
{
	size_t	available;

	if (length_status == BODY_ERROR)
		return (REQ_ERROR);
	if (length_status == BODY_CHUNKED)
		return (REQ_INCOMPLETE); // TODO #80/#82: defer to chunked completion
	if (length_status == BODY_NONE)
		return (REQ_COMPLETE);
	if (body_start > buffer.size())
		return (REQ_INCOMPLETE);
	available = buffer.size() - body_start;
	if (available < content_length)
		return (REQ_INCOMPLETE);
	return (REQ_COMPLETE); // overrun (>): extra bytes belong to the next request
}
