#include "log/log.hpp"
#include "config/Config.hpp"
#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"

Config::Config() : servers()
{
}

Config::~Config()
{
}

Config::Config(const Config & that) : servers(that.servers)
{
}

Config &	Config::operator=(const Config & that)
{
	if (this != &that)
		servers = that.servers;
	return (*this);
}

std::string	Config::readFile(const std::string & path)
{
	std::ifstream		file;
	std::stringstream	buffer;
	std::string			line;

	file.open(path.c_str());
	if (!file.is_open())
	{
		logError(std::string("cannot open config file: ") + path);
		std::exit(EXIT_FAILURE);
	}
	while (std::getline(file, line))
		buffer << line << "\n";
	file.close();
	return (buffer.str());
}

void	Config::removeComments(std::string & content)
{
	size_t	comment;
	size_t	start;
	size_t	end;

	start = 0;
	while (true)
	{
		comment = content.find('#', start);
		if (comment == std::string::npos)
			break;
		if (comment > 0 && content[comment - 1] == '\\')
		{
			start = comment + 1;
			continue;
		}
		end = content.find('\n', comment);
		if (end == std::string::npos)
			content.erase(comment);
		else
			content.erase(comment, end - comment);
		start = comment;
	}
}

void	Config::trim(std::string & str)
{
	size_t	start;
	size_t	end;

	start = 0;
	while (start < str.length() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r'))
		++start;
	end = str.length();
	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\n' || str[end - 1] == '\r'))
		--end;
	str = str.substr(start, end - start);
}

std::vector<std::string>	Config::splitLines(const std::string & content)
{
	std::vector<std::string>	lines;
	std::string					line;
	size_t						i;
	size_t						start;

	i = 0;
	while (i < content.length())
	{
		start = i;
		while (i < content.length() && content[i] != '\n')
			++i;
		line = content.substr(start, i - start);
		trim(line);
		if (!line.empty())
			lines.push_back(line);
		++i;
	}
	return (lines);
}

std::vector<std::string>	Config::tokenizeLine(const std::string & line)
{
	std::vector<std::string>	tokens;
	std::string					token;
	size_t						i;
	bool						inQuote;

	i = 0;
	inQuote = false;
	while (i < line.length())
	{
		if (line[i] == '"')
		{
			inQuote = !inQuote;
			++i;
			continue;
		}
		if (line[i] == ' ' || line[i] == '\t')
		{
			if (!token.empty() && !inQuote)
			{
				tokens.push_back(token);
				token.clear();
			}
		}
		else if (line[i] == ';')
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
			++i;
			continue;
		}
		else
		{
			token += line[i];
		}
		++i;
	}
	if (!token.empty())
		tokens.push_back(token);
	return (tokens);
}

void	Config::parseServers(const std::vector<std::string> & lines)
{
	size_t	i;

	i = 0;
	while (i < lines.size())
	{
		if (lines[i].find(WebServ::BLOCK_SERVER) == 0)
		{
			if (lines[i].find(WebServ::BLOCK_OPEN) != std::string::npos)
			{
				++i;
				servers.push_back(parseServerBlock(lines, i));
			}
			else if (i + 1 < lines.size() && lines[i + 1] == WebServ::BLOCK_OPEN)
			{
				i += 2;
				servers.push_back(parseServerBlock(lines, i));
			}
			else
				++i;
		}
		else
			++i;
	}
}

ServerConfig	Config::parseServerBlock(const std::vector<std::string> & lines, size_t & index)
{
	ServerConfig	server;
	std::string		location;
	size_t			braceCount;

	braceCount = 1;
	while (index < lines.size() && braceCount > 0)
	{
		if (lines[index] == WebServ::BLOCK_CLOSE)
		{
			--braceCount;
			if (braceCount == 0)
				break;
			++index;
			continue;
		}
		if (lines[index].find(WebServ::BLOCK_LOCATION) == 0)
		{
			if (lines[index].find(WebServ::BLOCK_OPEN) != std::string::npos)
			{
				location = lines[index];
				++index;
				server.addLocation(parseLocationBlock(lines, index, location));
			}
			else if (index + 1 < lines.size() && lines[index + 1] == WebServ::BLOCK_OPEN)
			{
				location = lines[index];
				index += 2;
				server.addLocation(parseLocationBlock(lines, index, location));
			}
			else
				++index;
			continue;
		}
		parseDirective(server, lines[index]);
		++index;
	}
	++index;
	return (server);
}

LocationConfig	Config::parseLocationBlock(const std::vector<std::string> & lines, size_t & index, std::string location)
{
	LocationConfig	locationConfig;
	size_t			braceCount;
	size_t			bracePos;
	std::string		locationPath;

	if (location.find(WebServ::BLOCK_OPEN) != std::string::npos)
	{
		bracePos = location.find(WebServ::BLOCK_OPEN);
		locationPath = location.substr(0, bracePos);
		trim(locationPath);
		if (locationPath.find(WebServ::BLOCK_LOCATION) == 0)
		{
			locationPath = locationPath.substr(8);
			trim(locationPath);
		}
	}
	else
	{
		locationPath = location;
		trim(locationPath);
		if (locationPath.find(WebServ::BLOCK_LOCATION) == 0)
		{
			locationPath = locationPath.substr(8);
			trim(locationPath);
		}
	}
	locationConfig.setPath(locationPath);
	braceCount = 1;
	while (index < lines.size() && braceCount > 0)
	{
		if (lines[index] == WebServ::BLOCK_CLOSE)
		{
			--braceCount;
			if (braceCount == 0)
				break;
			++index;
			continue;
		}
		parseLocationDirective(locationConfig, lines[index]);
		++index;
	}
	++index;
	return (locationConfig);
}

void	Config::parseDirective(ServerConfig & server, const std::string & line)
{
	std::vector<std::string>	tokens;
	size_t						i;
	std::string					sizeStr;
	size_t						multiplier;
	size_t						sizeValue;
	int							code;
	size_t						j;

	tokens = tokenizeLine(line);
	if (tokens.empty())
		return;
	if (tokens[0] == WebServ::D_LISTEN && tokens.size() >= 2)
	{
		i = 1;
		while (i < tokens.size())
		{
			server.addListenAddress(tokens[i]);
			++i;
		}
	}
	else if (tokens[0] == WebServ::D_SERVER && tokens.size() >= 2)
	{
		server.setServerName(tokens[1]);
	}
	else if (tokens[0] == WebServ::D_BODY_SIZE && tokens.size() >= 2)
	{
		sizeStr = tokens[1];
		multiplier = 1;
		if (!sizeStr.empty())
		{
			if (sizeStr[sizeStr.length() - 1] == 'm' || sizeStr[sizeStr.length() - 1] == 'M')
			{
				multiplier = 1048576;
				sizeStr = sizeStr.substr(0, sizeStr.length() - 1);
			}
			else if (sizeStr[sizeStr.length() - 1] == 'k' || sizeStr[sizeStr.length() - 1] == 'K')
			{
				multiplier = 1024;
				sizeStr = sizeStr.substr(0, sizeStr.length() - 1);
			}
		}
		std::istringstream iss(sizeStr);
		iss >> sizeValue;
		server.setClientMaxBodySize(sizeValue * multiplier);
	}
	else if (tokens[0] == WebServ::D_ERROR_PAGE && tokens.size() >= 3)
	{
		j = 1;
		while (j < tokens.size() - 1)
		{
			std::istringstream iss(tokens[j]);
			iss >> code;
			server.addErrorPage(code, tokens[tokens.size() - 1]);
			++j;
		}
	}
	else if (tokens[0] == WebServ::D_ROOT && tokens.size() >= 2)
	{
		server.setRoot(tokens[1]);
	}
	else if (tokens[0] == WebServ::D_INDEX && tokens.size() >= 2)
	{
		server.setIndex(tokens[1]);
	}
}

void	Config::parseLocationDirective(LocationConfig & location, const std::string & line)
{
	std::vector<std::string>	tokens;
	size_t						i;

	tokens = tokenizeLine(line);
	if (tokens.empty())
		return;
	if (tokens[0] == WebServ::D_METHODS)
	{
		i = 1;
		while (i < tokens.size())
		{
			location.addAllowedMethod(tokens[i]);
			++i;
		}
	}
	else if (tokens[0] == WebServ::D_REDIRECT && tokens.size() >= 3)
	{
		location.setRedirectCode(tokens[1]);
		location.setRedirect(tokens[2]);
	}
	else if (tokens[0] == WebServ::D_ROOT && tokens.size() >= 2)
	{
		location.setRoot(tokens[1]);
	}
	else if (tokens[0] == WebServ::D_AUTOINDEX && tokens.size() >= 2)
	{
		if (tokens[1] == WebServ::AUTOINDEX_ON)
			location.setAutoindex(true);
		else if (tokens[1] == WebServ::AUTOINDEX_OFF)
			location.setAutoindex(false);
	}
	else if (tokens[0] == WebServ::D_INDEX && tokens.size() >= 2)
	{
		location.setIndex(tokens[1]);
	}
	else if (tokens[0] == WebServ::D_STORE && tokens.size() >= 2)
	{
		location.setUploadStore(tokens[1]);
	}
	else if (tokens[0] == WebServ::D_CGI)
	{
		i = 1;
		while (i < tokens.size())
		{
			location.addCgiExtension(tokens[i]);
			++i;
		}
	}
}

void	Config::parse(const std::string & path)
{
	std::vector<std::string>	lines;
	std::string					content;

	log(std::string("configuration: ") + path);
	content = readFile(path);
	removeComments(content);
	lines = splitLines(content);
	parseServers(lines);
	validateConfig();
	logConfig(*this);
}

const std::vector<ServerConfig> &	Config::getServers() const
{
	return (servers);
}

void	Config::validateConfig()
{
	if (servers.empty())
	{
		logError("no server blocks found in config file");
		std::exit(EXIT_FAILURE);
	}
}
