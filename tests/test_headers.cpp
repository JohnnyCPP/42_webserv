#include "webserv.hpp"
#include "prototypes.hpp"

static int	g_pass = 0;
static int	g_fail = 0;

static const std::string	REQUEST_LINE = "GET / HTTP/1.1\r\n";

static void	report(const std::string &name, bool ok)
{
	if (ok)
	{
		++g_pass;
		std::cout << "PASS: " << name << std::endl;
	}
	else
	{
		++g_fail;
		std::cout << "FAIL: " << name << std::endl;
	}
}

static int	run(const std::string &section,
		std::map<std::string, std::string> &headers, size_t &body_start)
{
	std::string	buffer;

	buffer = REQUEST_LINE + section;
	return (parse_headers(buffer, REQUEST_LINE.size(), headers, body_start));
}

static bool	has(const std::map<std::string, std::string> &h,
		const std::string &key, const std::string &value)
{
	std::map<std::string, std::string>::const_iterator	it;

	it = h.find(key);
	return (it != h.end() && it->second == value);
}

int	main(void)
{
	std::map<std::string, std::string>	h;
	size_t								body;

	h.clear();
	report("single header",
		run("Host: example.com\r\n\r\n", h, body) == WebServ::OK
		&& h.size() == 1 && has(h, "host", "example.com"));

	h.clear();
	report("value OWS trimmed",
		run("Host:   example.com   \r\n\r\n", h, body) == WebServ::OK
		&& has(h, "host", "example.com"));

	h.clear();
	report("two headers + body offset",
		run("Content-Length: 5\r\nHost: a\r\n\r\nBODY", h, body) == WebServ::OK
		&& h.size() == 2 && has(h, "content-length", "5")
		&& has(h, "host", "a")
		&& body == REQUEST_LINE.size()
			+ std::string("Content-Length: 5\r\nHost: a\r\n\r\n").size());

	h.clear();
	report("internal space preserved",
		run("User-Agent: Mozilla 5.0\r\n\r\n", h, body) == WebServ::OK
		&& has(h, "user-agent", "Mozilla 5.0"));

	h.clear();
	report("empty value",
		run("X-Empty:\r\n\r\n", h, body) == WebServ::OK
		&& has(h, "x-empty", ""));

	h.clear();
	report("zero headers",
		run("\r\n", h, body) == WebServ::OK
		&& h.empty() && body == REQUEST_LINE.size() + 2);

	h.clear();
	report("incomplete (no blank line)",
		run("Host: example.com\r\n", h, body) == WebServ::REQUEST_INCOMPLETE);

	h.clear();
	report("incomplete (empty buffer)",
		parse_headers("", 0, h, body) == WebServ::REQUEST_INCOMPLETE);

	h.clear();
	report("no colon",
		run("Invalid line\r\n\r\n", h, body) == WebServ::BAD_REQUEST);

	h.clear();
	report("empty name",
		run(": value\r\n\r\n", h, body) == WebServ::BAD_REQUEST);

	h.clear();
	report("space before colon",
		run("Host : x\r\n\r\n", h, body) == WebServ::BAD_REQUEST);

	h.clear();
	report("line folding",
		run("Host: x\r\n folded\r\n\r\n", h, body) == WebServ::BAD_REQUEST);

	h.clear();
	report("conflicting content-length",
		run("Content-Length: 5\r\nContent-Length: 6\r\n\r\n", h, body)
		== WebServ::BAD_REQUEST);

	h.clear();
	report("duplicate last-wins",
		run("X-Test: a\r\nX-Test: b\r\n\r\n", h, body) == WebServ::OK
		&& has(h, "x-test", "b"));

	std::cout << "\n" << g_pass << " passed, " << g_fail << " failed."
		<< std::endl;
	return (g_fail == 0 ? 0 : 1);
}
