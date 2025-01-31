#pragma once

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <cstdlib>
#include "../utils/utils.hpp"
#include "../exceptions/exceptions.hpp"

#include "LocationConfig.hpp"

class LocationConfig;

/**
 * @brief class to store server specific info
 */
class ServerConfig
{
public:
	ServerConfig();
	~ServerConfig();

	/*** Getter ***/
	const std::string&					getHost( void ) const;
	const int&							getPort( void ) const;
	const std::vector<std::string>&		getServerNames( void ) const;
	const size_t&						getClientMaxBodySize( void ) const;
	const std::map<int, std::string>&	getErrorPages( void ) const;
	const std::vector<LocationConfig*>&	getLocations( void ) const; 

	/*** Setter ***/
	void	setHost( const std::string& host );
	void	setPort( const int& port );
	void	setServerNames( const std::vector<std::string>& serverNames );
	void	setClientMaxBodySize( const size_t& clientMaxBodySize );
	void	setErrorPages( const std::map<int, std::string> errorPages );
	void	setLocations( const std::vector<LocationConfig*>& locations );

	void	parseServerBlock( std::ifstream& file );


private:

	std::string						_host;
	int								_port;
	std::vector<std::string>		_serverNames;
	size_t							_clientMaxBodySize;
	std::map<int, std::string>		_errorPages;
	std::vector<LocationConfig*>	_locations;

	void	_addServerName( const std::string& serverName );
	void	_addErrorPage( const int& error, const std::string& errorPage );
	void	_addLocation( LocationConfig* location );

	size_t	_parseSize( const std::string& sizeStr );

	ServerConfig( const ServerConfig& other );
	ServerConfig& operator=( const ServerConfig& other );
};

std::ostream&	operator<<( std::ostream& os, const ServerConfig& server );