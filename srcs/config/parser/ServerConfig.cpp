#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
	: _host(), _port(0), _serverNames(), _clientMaxBodySize(0), _errorPages(), _locations() {}

ServerConfig::~ServerConfig()
{
	for (std::vector<LocationConfig*>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
		delete *it;
}

/*** Private ***/
ServerConfig::ServerConfig( const ServerConfig& other ) { (void)other; }

ServerConfig&	ServerConfig::operator=( const ServerConfig& other )
{
	(void)other;
	return *this;
}

/*** Getter ***/
const std::string&					ServerConfig::getHost( void ) const { return _host; }
const int&							ServerConfig::getPort( void ) const { return _port; }
const std::vector<std::string>&		ServerConfig::getServerNames( void ) const { return _serverNames; }
const size_t&						ServerConfig::getClientMaxBodySize( void ) const { return _clientMaxBodySize; }
const std::map<int, std::string>&	ServerConfig::getErrorPages( void ) const { return _errorPages; }
const std::vector<LocationConfig*>&	ServerConfig::getLocations( void ) const { return _locations; }

/*** Setter ***/
void	ServerConfig::setHost( const std::string& host ) { _host = host; }
void	ServerConfig::setPort( const int& port ) { _port = port; }
void	ServerConfig::setServerNames( const std::vector<std::string>& serverNames ) { _serverNames = serverNames; }
void	ServerConfig::setClientMaxBodySize( const size_t& clientMaxBodySize ) { _clientMaxBodySize = clientMaxBodySize; }
void	ServerConfig::setErrorPages( const std::map<int, std::string> errorPages ) { _errorPages = errorPages; }
void	ServerConfig::setLocations( const std::vector<LocationConfig*>& locations ) { _locations = locations; }

/*** private helper methods ***/

/**
 * @brief private method to add a server name string to the class _serverNames vector
 * 
 * @param serverName a string containign the name used for the server
 */
void	ServerConfig::_addServerName( const std::string& serverName )
{
	_serverNames.push_back(serverName);
}

/**
 * @brief private method to add a key-value map <int, string> to the class _errorPages map
 * 
 * @param error the integer value of the error
 * @param errorPage the path to the corresponding error page
 */
void	ServerConfig::_addErrorPage( const int& error, const std::string& errorPage )
{
	_errorPages.insert(std::make_pair(error, errorPage));
}

/**
 * @brief private method to add a LocationConfig object to the class _locations vector
 * 
 * @param LocationConfig the object containing all location-block info
 */
void	ServerConfig::_addLocation( LocationConfig* location )
{
	_locations.push_back(location);
}

/**
 * @brief Converts a size string (e.g., "1M") to an integer in bytes.
 * 
 * Supports:
 *  - "1024"  →  1024 bytes
 *  - "1K"    →  1024 bytes
 *  - "1M"    →  1,048,576 bytes
 * 
 * @param sizeStr The input size string.
 * @return size_t The size in bytes.
 */
size_t	ServerConfig::_parseSize( const std::string& sizeStr )
{
	if (sizeStr.empty())
		throw ConfigException("client_max_body_size: Invalid size (empty value).");
	
	std::string trimmedStr = trim(sizeStr, whiteSpaces + ";");
	size_t		multiplier = 1;
	std::string	numPart = sizeStr;

	char lastChar = trimmedStr[trimmedStr.size() - 1];
	if (lastChar == 'K' || lastChar == 'k')
	{
		multiplier = 1024;
		numPart = trimmedStr.substr(0, trimmedStr.size() - 1);
	}
	else if (lastChar == 'M' || lastChar == 'm')
	{
		multiplier = 1024 * 1024;
		numPart = trimmedStr.substr(0, trimmedStr.size() - 1);
	}

	char*	endPtr;
	long	result = strtol(numPart.c_str(), &endPtr, 10);

	// Validate conversion
	if (*endPtr != '\0' || result < 0)
		throw ConfigException("client_max_body_size: Invalid number format '" + trimmedStr + "'.");

	return static_cast<size_t>(result) * multiplier;
}

/*** public parser method ***/

/**
 * @brief Parses a `server` block from the configuration file.
 */
void ServerConfig::parseServerBlock(std::ifstream& file)
{
	std::string line;

	while (std::getline(file, line))
	{
		line = trim(line, whiteSpaces);
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream iss(line);
		std::string key;
		iss >> key;

		if (key == "listen")
		{
			std::string listenValue = extractDirectiveValue(line, key);
			
			size_t colon = listenValue.find(':');
			if (colon != std::string::npos)
			{
				setHost(listenValue.substr(0, colon));
				setPort(atoi(listenValue.substr(colon + 1).c_str()));
			}
			else
				throw ConfigException("Invalid 'listen' format, expected 'IP:PORT'.");
		}
		else if (key == "server_name")
		{
			std::string value = extractDirectiveValue(line, key);
			std::vector<std::string> names = split(value, SPACE);
			setServerNames(names);
		}
		else if (key == "client_max_body_size")
		{
			std::string value = extractDirectiveValue(line, key);
			setClientMaxBodySize(_parseSize(value));
		}
		else if (key == "error_page")
		{
			std::string value = extractDirectiveValue(line, key);
			std::istringstream valueIss(value);
			
			int errorCode;
			std::string errorPage;
			
			valueIss >> errorCode >> errorPage;
			
			if (valueIss.fail())
				throw ConfigException("Failed to parse error_page directive");
			
			_addErrorPage(errorCode, errorPage);
		}
		else if (key == "location")
		{  
			std::string	path;
			iss >> path;
			LocationConfig* location = new LocationConfig();
			location->setPath(path);
			location->parseLocationBlock(file);
			_addLocation(location);
		}
		else if (key == "}")
			return;
		else
			throw ConfigException("Unknown directive inside 'server': " + key);
	}

	throw ConfigException("Missing closing '}' for server block.");
}

/**
 * @brief << operator overload
 */
std::ostream&	operator<<( std::ostream& os, const ServerConfig& server )
{
	os << "    ServerConfig {" << std::endl;
	os << "            Host: " << server.getHost() << std::endl;
	os << "            Port: " << server.getPort() << std::endl;
	
	os << "            Server Names: ";
	for (std::vector<std::string>::const_iterator it = server.getServerNames().begin(); it != server.getServerNames().end(); ++it)
		os << *it << " ";
	os << std::endl;

	os << "            Client Max Body Size: " << server.getClientMaxBodySize() << std::endl;
	
	os << "            Error Pages: ";
	for (std::map<int, std::string>::const_iterator it = server.getErrorPages().begin(); it != server.getErrorPages().end(); ++it)
		os << "[" << it->first << "]: " << it->second << " ";
	os << std::endl;

	os << "            Locations: " << std::endl;
	for (std::vector<LocationConfig*>::const_iterator it = server.getLocations().begin(); it != server.getLocations().end(); ++it)
		os << "        " << **it << std::endl;

	os << "        }" << std::endl;
	return os;
}