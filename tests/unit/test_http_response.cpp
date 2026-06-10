// Unit tests for HttpResponse (src/http/HttpResponse.cpp).
//
// The eval insists every status code must be correct, so these lock down the
// status line, default reason phrases, Content-Length bookkeeping, content-type
// mapping and the factory helpers Phase 3 will rely on.

#include "http/HttpResponse.hpp"
#include "test_util.hpp"

static bool	contains(const std::string & hay, const std::string & needle)
{
	return (hay.find(needle) != std::string::npos);
}

// Extract a single header value from a serialized response ("Key: value\r\n").
static std::string	headerValue(const std::string & raw, const std::string & key)
{
	std::string	prefix;
	size_t		start;
	size_t		end;

	prefix = key + ": ";
	start = raw.find(prefix);
	if (start == std::string::npos)
		return ("");
	start += prefix.size();
	end = raw.find("\r\n", start);
	if (end == std::string::npos)
		return ("");
	return (raw.substr(start, end - start));
}

int	main()
{
	std::cout << "== HttpResponse unit tests ==" << std::endl;

	// Default state
	{
		HttpResponse	r;
		test::equal_int(200, r.getStatusCode(), "default status code is 200");
		test::check(contains(r.toString(), "HTTP/1.1 200 OK\r\n"),
			"default status line is 'HTTP/1.1 200 OK'");
	}

	// setStatus maps to the correct reason phrase
	{
		HttpResponse	r;
		r.setStatus(404);
		test::check(contains(r.toString(), "HTTP/1.1 404 Not Found\r\n"),
			"setStatus(404) yields '404 Not Found'");
		r.setStatus(500);
		test::check(contains(r.toString(), "500 Internal Server Error"),
			"setStatus(500) yields 'Internal Server Error'");
	}

	// setBody updates Content-Length
	{
		HttpResponse	r;
		r.setBody("hello");
		test::equal("5", headerValue(r.toString(), "Content-Length"),
			"setBody sets Content-Length to body size");
	}

	// Serialization layout: status line, headers, blank line, body
	{
		HttpResponse	r;
		std::string		s;
		r.setStatus(200);
		r.setBody("BODY");
		s = r.toString();
		test::check(contains(s, "\r\n\r\nBODY"),
			"toString separates headers and body with a blank line");
	}

	// ok() factory
	{
		HttpResponse	r = HttpResponse::ok("page");
		test::equal_int(200, r.getStatusCode(), "ok() is 200");
		test::equal("4", headerValue(r.toString(), "Content-Length"),
			"ok() sets Content-Length");
	}

	// notFound() factory
	{
		HttpResponse	r = HttpResponse::notFound();
		test::equal_int(404, r.getStatusCode(), "notFound() is 404");
		test::check(contains(r.toString(), "<html"),
			"notFound() carries an HTML body");
	}

	// methodNotAllowed() must advertise allowed methods via Allow header
	{
		HttpResponse	r = HttpResponse::methodNotAllowed("GET, POST");
		test::equal_int(405, r.getStatusCode(), "methodNotAllowed() is 405");
		test::equal("GET, POST", headerValue(r.toString(), "Allow"),
			"methodNotAllowed() sets the Allow header");
	}

	// created() must carry a Location header
	{
		HttpResponse	r = HttpResponse::created("/uploads/file.txt");
		test::equal_int(201, r.getStatusCode(), "created() is 201");
		test::equal("/uploads/file.txt", headerValue(r.toString(), "Location"),
			"created() sets the Location header");
	}

	// redirect factory carries Location
	{
		HttpResponse	r = HttpResponse::movedPermanently("/new");
		test::equal_int(301, r.getStatusCode(), "movedPermanently() is 301");
		test::equal("/new", headerValue(r.toString(), "Location"),
			"movedPermanently() sets the Location header");
	}

	// payloadTooLarge factory
	{
		HttpResponse	r = HttpResponse::payloadTooLarge();
		test::equal_int(413, r.getStatusCode(), "payloadTooLarge() is 413");
	}

	// Content-Type mapping by extension
	{
		HttpResponse	r;
		r.setContentType(".html");
		test::equal("text/html", headerValue(r.toString(), "Content-Type"),
			".html -> text/html");
		r.setContentType(".css");
		test::equal("text/css", headerValue(r.toString(), "Content-Type"),
			".css -> text/css");
		r.setContentType(".png");
		test::equal("image/png", headerValue(r.toString(), "Content-Type"),
			".png -> image/png");
		r.setContentType(".weird");
		test::equal("application/octet-stream",
			headerValue(r.toString(), "Content-Type"),
			"unknown extension -> application/octet-stream");
	}

	return (test::report("HttpResponse"));
}
