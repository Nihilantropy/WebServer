#include "ConfParser.hpp"

/*** Canonichal form ***/

ConfParser::ConfParser( const std::string& filename ) : _filename(filename), _configFile(), _servers()
{
	_openFile();
}

ConfParser::~ConfParser()
{
	for (std::vector<ServerConfig*>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
		delete *it;
}

/*** Private ***/
ConfParser::ConfParser( const ConfParser& other ) { (void)other; }

ConfParser&	ConfParser::operator=( const ConfParser& other )
{
	(void)other;
	return *this;
}

/*** Getter ***/
const std::string&					ConfParser::getFilename( void ) const { return _filename; }
const std::vector<ServerConfig*>&	ConfParser::getServers( void ) const { return _servers; }

/*** Setter ***/
void	ConfParser::setFilename( const std::string& filename ) { _filename = filename; }
void	ConfParser::setServers( const std::vector<ServerConfig*>& servers ) { _servers = servers; }

/*** private helper methods ***/

void	ConfParser::_openFile( void )
{
    _configFile.open(_filename.c_str());
    if (!_configFile.is_open())
	{
        throw ConfigException("Failed to open " + _filename + ": " + strerror(errno));
    }
}

/**
 * @brief private method to add a server object to the class _servers vector
 * 
 * @param ServerConfig an object containing all necessary server info
 */
void	ConfParser::_addServer( ServerConfig* server )
{
	_servers.push_back(server);
}

/*** public parser method ***/

/**
 * @brief Main parser method for the .conf file
 */
void	ConfParser::parseConfigFile( void )
{
	std::string	line;
	
	while (std::getline(_configFile, line))
	{
		line = trim(line, whiteSpaces);
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream iss(line);
		std::string key;
		iss >> key;

		if (key == "server")
		{
			ServerConfig*	server = new ServerConfig;
			server->parseServerBlock(_configFile);
			_addServer(server);
		}
		else
			throw ConfigException("Unexpected directive outside of server block: " + key);
	}
}

/**
 * @brief << operator overload
 */
std::ostream&	operator<<( std::ostream& os, const ConfParser& parser )
{
	os << "ConfParser {" << std::endl;
	os << "    Filename: " << parser.getFilename() << std::endl;
	os << "    Servers: " << std::endl;

	for (std::vector<ServerConfig*>::const_iterator it = parser.getServers().begin(); it != parser.getServers().end(); ++it)
		os << "    " << **it << std::endl;

	os << "}" << std::endl;
	return os;
}
