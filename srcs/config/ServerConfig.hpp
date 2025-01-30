#pragma once

#include <string>
#include <iostream>
#include <map>
#include <vector>

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
	const int&							getProt( void ) const;
	const std::vector<std::string>&		getServerNames( void ) const;
	const size_t&						getClientMaxBodySize( void ) const;
	const std::map<int, std::string>&	getErrorPages( void ) const;

	/*** Setter ***/
	void	setHost( const std::string& host );
	void	setPort( const int& port );
	void	setServerNames( const std::vector<std::string>& serverNames );
	void	setClientMaxBodySize( const size_t& clientMaxBodySize );
	void	setErrorPages( const std::map<int, std::string> errorPages );
	void	setLocations( const std::vector<LocationConfig>& locations );

	void	parseServerBlock( const std::istream& file );


private:

	std::string					_host;
	int							_port;
	std::vector<std::string>	_serverNames;
	size_t						_clientMaxBodySize;
	std::map<int, std::string>	_errorPages;
	std::vector<LocationConfig>	_locations;

	void	addServerName( const std::string& serverName );
	void	addErrorPage( const int& error, const std::string& errorPage );
	void	addLocation( const LocationConfig& location );

	ServerConfig( const ServerConfig& other );
	ServerConfig& operator=( const ServerConfig& other );
};