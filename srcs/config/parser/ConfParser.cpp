#include "ConfParser.hpp"
#include <sstream>

ConfParser::ConfParser(const std::string& filename) : _filename(filename), _configFile(), _servers()
{
	_openFile();
	_parseConfigFile();
	_setDefaults();
	_validate();
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
        throw OpenException(strerror(errno));
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

/*** private parser methods ***/

/**
 * @brief Main parser method for the .conf file
 */
void	ConfParser::_parseConfigFile( void )
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
 * @brief Validates all server configurations
 *
 * This method creates ServerConfigValidator instances for each
 * ServerConfig object to ensure all configurations are valid.
 */
void	ConfParser::_validate(void)
{
	if (_servers.empty()) {
		throw ValidationException("No server blocks found in configuration file");
	}

	// Create a validator for each server config
	for (std::vector<ServerConfig*>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
		// ServerConfigValidator's constructor will call _validate()
		ServerConfigValidator validator(**it);
	}

	// Check for duplicate server configurations (host:port combinations)
	std::map<std::string, std::vector<ServerConfig*> > hostPortMap;
	
	for (std::vector<ServerConfig*>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
		std::stringstream ss;
		ss << (*it)->getHost() << ":" << (*it)->getPort();
		std::string hostPort = ss.str();
		hostPortMap[hostPort].push_back(*it);
	}

	// Verify that the first server for each host:port has server_names
	for (std::map<std::string, std::vector<ServerConfig*> >::const_iterator it = hostPortMap.begin(); it != hostPortMap.end(); ++it) {
		if (it->second.size() > 1) {
			// Multiple servers for this host:port
			// Check if the first one has server_names
			if (it->second[0]->getServerNames().empty()) {
				throw ValidationException("Default server for " + it->first + " must have server_names");
			}
			
			// Check for duplicate server names across all servers on this host:port
			std::set<std::string> serverNames;
			for (std::vector<ServerConfig*>::const_iterator serverIt = it->second.begin(); serverIt != it->second.end(); ++serverIt) {
				const std::vector<std::string>& names = (*serverIt)->getServerNames();
				for (std::vector<std::string>::const_iterator nameIt = names.begin(); nameIt != names.end(); ++nameIt) {
					if (serverNames.find(*nameIt) != serverNames.end()) {
						throw ValidationException("Duplicate server_name '" + *nameIt + "' for " + it->first);
					}
					serverNames.insert(*nameIt);
				}
			}
		}
	}
}

/**
 * @brief Sets default values for all server and location configurations
 */
void	ConfParser::_setDefaults(void)
{
	for (std::vector<ServerConfig*>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		// Set default values for the server configuration
		ServerConfigDefaults::setDefaults(**it);
		
		// Set default values for each location configuration
		const std::vector<LocationConfig*>& locations = (*it)->getLocations();
		for (std::vector<LocationConfig*>::const_iterator locIt = locations.begin(); locIt != locations.end(); ++locIt) {
			LocationConfigDefaults::setDefaults(**locIt);
		}
	}
}

/**
 * @brief << operator overload
 */
std::ostream&	operator<<( std::ostream& os, const ConfParser& parser )
{
	os << "ConfParser {" << std::endl;
	os << "    Filename: " << parser.getFilename() << std::endl;
	os << "        Servers: " << std::endl;

	for (std::vector<ServerConfig*>::const_iterator it = parser.getServers().begin(); it != parser.getServers().end(); ++it)
		os << "    " << **it << std::endl;

	os << "}" << std::endl;
	return os;
}