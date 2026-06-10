// Unit tests for the live request parser (src/client/Client.cpp).
//
// The parser is fed raw bytes through appendToBuffer()/parseRequest(), exactly
// as WebServer::handleClientRead does, and we assert on the parsed fields and
// completion/error flags.

#include "client/Client.hpp"
#include "test_util.hpp"

// Build a Client, feed it raw bytes, run the parse step.
static Client	parsed(const std::string & raw)
{
	Client	c(3);
	c.appendToBuffer(raw);
	c.parseRequest();
	return (c);
}

int	main()
{
	std::cout << "== Client parser unit tests ==" << std::endl;

	// Simple complete GET (no body)
	{
		Client	c = parsed("GET /index.html HTTP/1.1\r\nHost: a\r\n\r\n");
		test::equal("GET", c.getMethod(), "GET: method parsed");
		test::equal("/index.html", c.getPath(), "GET: path parsed");
		test::equal("HTTP/1.1", c.getVersion(), "GET: version parsed");
		test::check(c.isHeadersComplete(), "GET: headers marked complete");
		test::check(c.isRequestComplete(), "GET: request marked complete");
		test::check(!c.hasError(), "GET: no error flagged");
	}

	// Header values are extracted and left-trimmed
	{
		Client	c = parsed("GET / HTTP/1.1\r\nHost: example.com\r\nUser-Agent: curl/8\r\n\r\n");
		const std::map<std::string, std::string> & h = c.getHeaders();
		test::check(h.find("Host") != h.end(), "headers: Host present");
		if (h.find("Host") != h.end())
			test::equal("example.com", h.find("Host")->second,
				"headers: Host value trimmed");
		test::check(h.find("User-Agent") != h.end(),
			"headers: multiple headers parsed");
	}

	// POST with Content-Length body fully present
	{
		Client	c = parsed("POST /upload HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\n\r\nhello");
		test::equal("POST", c.getMethod(), "POST: method parsed");
		test::equal_int(5, (long)c.getContentLength(), "POST: Content-Length detected");
		test::check(c.isRequestComplete(), "POST: complete when full body present");
		test::equal("hello", c.getBody(), "POST: body extracted");
	}

	// Body not fully arrived yet -> not complete, not error
	{
		Client	c = parsed("POST /u HTTP/1.1\r\nContent-Length: 10\r\n\r\nabc");
		test::check(!c.isRequestComplete(),
			"partial body: request not complete until all bytes arrive");
		test::check(!c.hasError(), "partial body: not an error");
	}

	// Headers not terminated -> not complete, not error (waiting for more)
	{
		Client	c = parsed("GET / HTTP/1.1\r\nHost: a\r\n");
		test::check(!c.isHeadersComplete(),
			"incomplete headers: not complete (no blank line yet)");
		test::check(!c.hasError(), "incomplete headers: not an error");
	}

	// Chunked transfer encoding is detected
	{
		Client	c = parsed("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n");
		test::check(c.isChunked(), "Transfer-Encoding: chunked detected");
	}

	// Invalid method (lowercase) flags an error
	{
		Client	c = parsed("get / HTTP/1.1\r\nHost: a\r\n\r\n");
		test::check(c.hasError(), "invalid method 'get' flagged as error");
	}

	// Unsupported version flags an error
	{
		Client	c = parsed("GET / HTTP/2.0\r\nHost: a\r\n\r\n");
		test::check(c.hasError(), "unsupported version HTTP/2.0 flagged as error");
	}

	// Malformed request line (missing space/version) flags an error
	{
		Client	c = parsed("GET /onlytwo\r\nHost: a\r\n\r\n");
		test::check(c.hasError(), "request line without version flagged as error");
	}

	// resetForNextRequest clears parsed state
	{
		Client	c = parsed("GET /a HTTP/1.1\r\nHost: a\r\n\r\n");
		c.resetForNextRequest();
		test::check(c.getMethod().empty() && c.getPath().empty(),
			"resetForNextRequest clears method and path");
		test::check(!c.isRequestComplete(),
			"resetForNextRequest clears completion flag");
	}

	return (test::report("Client parser"));
}
