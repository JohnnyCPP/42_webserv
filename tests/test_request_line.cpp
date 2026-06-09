#include "request.hpp"

static int	g_pass = 0;
static int	g_fail = 0;

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

int	main(void)
{
	RequestLine	rl;
	size_t		consumed;
	int			status;

	// simple request line, no query
	status = parse_request_line("GET /index.html HTTP/1.1\r\n", rl, consumed);
	report("simple GET status OK", status == WebServ::OK);
	report("method", rl.getMethod() == "GET");
	report("path", rl.getPath() == "/index.html");
	report("empty query", rl.getQuery() == "");
	report("version", rl.getVersion() == "HTTP/1.1");
	report("consumed full line",
		consumed == std::string("GET /index.html HTTP/1.1\r\n").size());

	// query string split off the path
	RequestLine	q;
	report("query parsed",
		parse_request_line("GET /search?name=cat&x=1 HTTP/1.1\r\n", q, consumed)
		== WebServ::OK
		&& q.getPath() == "/search" && q.getQuery() == "name=cat&x=1");

	// incomplete: no CRLF yet
	RequestLine	inc;
	report("incomplete without CRLF",
		parse_request_line("GET / HTTP/1.1", inc, consumed)
		== WebServ::REQUEST_INCOMPLETE);

	// malformed: only two tokens
	RequestLine	bad;
	report("missing version -> BAD_REQUEST",
		parse_request_line("GET /\r\n", bad, consumed) == WebServ::BAD_REQUEST);

	// path must start with '/'
	RequestLine	rel;
	report("relative path -> BAD_REQUEST",
		parse_request_line("GET index.html HTTP/1.1\r\n", rel, consumed)
		== WebServ::BAD_REQUEST);

	// unsupported HTTP version
	RequestLine	ver;
	report("HTTP/2.0 -> VERSION_NOT_SUPPORTED",
		parse_request_line("GET / HTTP/2.0\r\n", ver, consumed)
		== WebServ::VERSION_NOT_SUPPORTED);

	// OCF: copy constructor and assignment preserve all fields
	RequestLine	original;
	parse_request_line("POST /up?a=b HTTP/1.1\r\n", original, consumed);
	RequestLine	copy(original);
	RequestLine	assigned;
	assigned = original;
	report("copy ctor preserves fields",
		copy.getMethod() == "POST" && copy.getPath() == "/up"
		&& copy.getQuery() == "a=b" && copy.getVersion() == "HTTP/1.1");
	report("assignment preserves fields",
		assigned.getMethod() == "POST" && assigned.getPath() == "/up"
		&& assigned.getQuery() == "a=b" && assigned.getVersion() == "HTTP/1.1");

	std::cout << "\n" << g_pass << " passed, " << g_fail << " failed."
		<< std::endl;
	return (g_fail == 0 ? 0 : 1);
}
