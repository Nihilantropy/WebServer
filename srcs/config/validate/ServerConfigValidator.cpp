#include "ServerConfigValidator.hpp"

ServerConfigValidator::ServerConfigValidator( const ServerConfig& serverConfig) : _serverConfig(serverConfig)
{
	_validate();
}

void	ServerConfigValidator::_validate( void ) const
{
	// server port can be multiple
	// server name can be customized or not - set default name - check how nginx handle server_name
	// setup default error pages if none are defined
	// limit clientbody size
	// Set a default file to answer if the request is a directory.
}