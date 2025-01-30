#include "ConfParser.hpp"

/*** Canonichal form ***/

ConfParser::ConfParser( const std::string& filename ) : _filename(filename), _servers(NULL) {}

ConfParser::~ConfParser() {}

/*** Private ***/
ConfParser::ConfParser( const ConfParser& other ) { (void)other; }

ConfParser&	ConfParser::operator=( const ConfParser& other )
{
	(void)other;
	return *this;
}

/*** Getter ***/
const std::string&					ConfParser::getFilename( void ) const { return _filename; }
const std::vector<ServerConfig>&	ConfParser::getServers( void ) const { return _servers; }

/*** Setter ***/
void	ConfParser::setServers( const std::vector<ServerConfig> servers ) { _servers = servers; }

/*** private helper methods ***/

/**
 * @brief private method to add a server object to the class _servers vector
 * 
 * @param ServerConfig an object containing all necessary server info
 */
void	ConfParser::addServer( const ServerConfig& server )
{
	_servers.push_back(server);
}


/**
 * @brief main parser method for the .conf file
 */
void	ConfParser::parseConfigFile( void )
{

}
