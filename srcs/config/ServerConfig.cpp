#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
	: _host(NULL), _port(0), _serverNames(NULL), _clientMaxBodySize(0), _errorPages(), _locations(NULL) {}

ServerConfig::~ServerConfig() {}

/*** Private ***/
ServerConfig::ServerConfig( const ServerConfig& other ) { (void)other; }

ServerConfig&	ServerConfig::operator=( const ServerConfig& other )
{
	(void)other;
	return *this;
}

/*** Getter ***/
const std::string&					ServerConfig::getHost( void ) const { return _host; }
const int&							ServerConfig::getProt( void ) const { return _port; }
const std::vector<std::string>&		ServerConfig::getServerNames( void ) const { return _serverNames; }
const size_t&						ServerConfig::getClientMaxBodySize( void ) const { return _clientMaxBodySize; }
const std::map<int, std::string>&	ServerConfig::getErrorPages( void ) const { return _errorPages; }

/*** Setter ***/
void	ServerConfig::setHost( const std::string& host ) { _host = host; }
void	ServerConfig::setPort( const int& port ) { _port = port; }
void	ServerConfig::setServerNames( const std::vector<std::string>& serverNames ) { _serverNames = serverNames; }
void	ServerConfig::setClientMaxBodySize( const size_t& clientMaxBodySize ) { _clientMaxBodySize = clientMaxBodySize; }
void	ServerConfig::setErrorPages( const std::map<int, std::string> errorPages ) { _errorPages = errorPages; }
void	ServerConfig::setLocations( const std::vector<LocationConfig>& locations ) { _locations = locations; }

/*** private helper methods ***/

/**
 * @brief private method to add a server name string to the class _serverNames vector
 * 
 * @param serverName a string containign the name used for the server
 */
void	ServerConfig::addServerName( const std::string& serverName )
{
	_serverNames.push_back(serverName);
}

/**
 * @brief private method to add a key-value map <int, string> to the class _errorPages map
 * 
 * @param error the integer value of the error
 * @param errorPage the path to the corresponding error page
 */
void	ServerConfig::addErrorPage( const int& error, const std::string& errorPage )
{
	_errorPages.insert(std::make_pair(error, errorPage));
}

/**
 * @brief private method to add a LocationConfig object to the class _locations vector
 * 
 * @param LocationConfig the object containing all location-block info
 */
void	ServerConfig::addLocation( const LocationConfig& location )
{
	_locations.push_back(location);
}

/*** public parser method ***/

/**
 * @brief parser method to get all server-info from .conf file
 */
void	ServerConfig::parseServerBlock( const std::istream& file )
{

}