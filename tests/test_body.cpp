#include "webserv.hpp"
#include "prototypes.hpp"

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

static std::map<std::string, std::string>	one(const std::string &key,
		const std::string &value)
{
	std::map<std::string, std::string>	h;

	h[key] = value;
	return (h);
}

int	main(void)
{
	std::map<std::string, std::string>	h;
	size_t								len;

	len = 42;
	report("CL 5 -> LENGTH 5",
		detect_body_length(one("content-length", "5"), len) == BODY_LENGTH
		&& len == 5);

	len = 42;
	report("CL 0 -> LENGTH 0",
		detect_body_length(one("content-length", "0"), len) == BODY_LENGTH
		&& len == 0);

	report("no headers -> NONE",
		detect_body_length(std::map<std::string, std::string>(), len)
		== BODY_NONE);

	report("chunked -> CHUNKED",
		detect_body_length(one("transfer-encoding", "chunked"), len)
		== BODY_CHUNKED);

	h.clear();
	h["content-length"] = "5";
	h["transfer-encoding"] = "chunked";
	report("chunked wins over CL",
		detect_body_length(h, len) == BODY_CHUNKED);

	report("CL abc -> ERROR",
		detect_body_length(one("content-length", "abc"), len) == BODY_ERROR);

	report("CL -1 -> ERROR",
		detect_body_length(one("content-length", "-1"), len) == BODY_ERROR);

	report("CL empty -> ERROR",
		detect_body_length(one("content-length", ""), len) == BODY_ERROR);

	report("NONE -> COMPLETE",
		check_request_complete("", 0, BODY_NONE, 0) == REQ_COMPLETE);

	report("LENGTH 5, 3 bytes -> INCOMPLETE",
		check_request_complete("abc", 0, BODY_LENGTH, 5) == REQ_INCOMPLETE);

	report("LENGTH 5, 5 bytes -> COMPLETE",
		check_request_complete("abcde", 0, BODY_LENGTH, 5) == REQ_COMPLETE);

	report("LENGTH 5, 7 bytes -> COMPLETE",
		check_request_complete("abcdefg", 0, BODY_LENGTH, 5) == REQ_COMPLETE);

	report("LENGTH 0, 0 bytes -> COMPLETE",
		check_request_complete("", 0, BODY_LENGTH, 0) == REQ_COMPLETE);

	report("ERROR -> ERROR",
		check_request_complete("abc", 0, BODY_ERROR, 0) == REQ_ERROR);

	report("bodyStart past end -> INCOMPLETE",
		check_request_complete("abc", 10, BODY_LENGTH, 5) == REQ_INCOMPLETE);

	std::cout << "\n" << g_pass << " passed, " << g_fail << " failed."
		<< std::endl;
	return (g_fail == 0 ? 0 : 1);
}
