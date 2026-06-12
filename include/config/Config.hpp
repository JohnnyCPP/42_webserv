#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "webserv.hpp"

class LocationConfig;
class ServerConfig;

class Config
{
private:

	std::vector<ServerConfig>	servers;

	std::string							readFile(const std::string & path);
	void								removeComments(std::string & content);
	void								normalizeBraces(std::string & content);
	void								trim(std::string & str);
	std::vector<std::string>			splitLines(const std::string & content);
	void								parseServers(const std::vector<std::string> & lines);
	ServerConfig						parseServerBlock(const std::vector<std::string> & lines, size_t & index);
	LocationConfig						parseLocationBlock(const std::vector<std::string> & lines, size_t & index, std::string location);
	void								parseDirective(ServerConfig & server, const std::string & line);
	void								parseLocationDirective(LocationConfig & location, const std::string & line);
	std::vector<std::string>			tokenizeLine(const std::string & line);
	void								validateConfig();

public:

	Config();
	~Config();
	Config(const Config & that);
	Config & operator=(const Config & that);

	void								parse(const std::string & path);
	const std::vector<ServerConfig> &	getServers() const;
};

#endif
