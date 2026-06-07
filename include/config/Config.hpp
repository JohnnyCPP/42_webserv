#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "webserv.hpp"

class LocationConfig;
class ServerConfig;

class Config
{
private:

	std::vector<ServerConfig>	servers;

	std::string					readFile(std::string const & path);
	void						removeComments(std::string & content);
	void						normalizeBraces(std::string & content);
	void						trim(std::string & str);
	std::vector<std::string>	splitLines(std::string const & content);
	void						parseServers(std::vector<std::string> const & lines);
	ServerConfig				parseServerBlock(std::vector<std::string> const & lines, size_t & index);
	LocationConfig				parseLocationBlock(std::vector<std::string> const & lines, size_t & index, std::string location);
	void						parseDirective(ServerConfig & server, std::string const & line);
	void						parseLocationDirective(LocationConfig & location, std::string const & line);
	std::vector<std::string>	tokenizeLine(std::string const & line);
	void						validateConfig();

public:

	Config();
	Config(Config const & that);
	~Config();
	Config & operator=(Config const & that);

	void								parse(std::string const & path);
	std::vector<ServerConfig> const &	getServers() const;
};

#endif
