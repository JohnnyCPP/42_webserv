#ifndef LOG_HPP
# define LOG_HPP

# include "webserv.hpp"
# include "config/Config.hpp"
# include "config/ServerConfig.hpp"
# include "config/LocationConfig.hpp"

void	log(const std::string & message);
void	logError(const std::string & message);
void	logConfig(const Config & config);

#endif
