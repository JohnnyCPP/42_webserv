#ifndef TEST_UTIL_HPP
# define TEST_UTIL_HPP

// Tiny self-contained assertion helpers for the unit tests.
// No framework, no allocation, c++98-clean.

# include <iostream>
# include <string>

namespace test
{
	static int	g_pass = 0;
	static int	g_fail = 0;

	inline void	check(bool cond, const std::string & name)
	{
		if (cond)
		{
			++g_pass;
			std::cout << "  \033[0;32mPASS\033[0m  " << name << std::endl;
		}
		else
		{
			++g_fail;
			std::cout << "  \033[0;31mFAIL\033[0m  " << name << std::endl;
		}
	}

	inline void	equal(const std::string & expected, const std::string & actual,
				const std::string & name)
	{
		if (expected == actual)
		{
			++g_pass;
			std::cout << "  \033[0;32mPASS\033[0m  " << name << std::endl;
		}
		else
		{
			++g_fail;
			std::cout << "  \033[0;31mFAIL\033[0m  " << name
				<< "  (expected \"" << expected << "\", got \""
				<< actual << "\")" << std::endl;
		}
	}

	inline void	equal_int(long expected, long actual, const std::string & name)
	{
		if (expected == actual)
		{
			++g_pass;
			std::cout << "  \033[0;32mPASS\033[0m  " << name << std::endl;
		}
		else
		{
			++g_fail;
			std::cout << "  \033[0;31mFAIL\033[0m  " << name
				<< "  (expected " << expected << ", got " << actual
				<< ")" << std::endl;
		}
	}

	inline int	report(const std::string & suite)
	{
		std::cout << "\n" << suite << ": "
			<< g_pass << " passed, " << g_fail << " failed" << std::endl;
		return (g_fail == 0 ? 0 : 1);
	}
}

#endif
