#include "webserv.hpp"

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
