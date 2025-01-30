#pragma once

#include <iostream>
#include <vector>


#include "ServerConfig.hpp"

class ServerConfig;

/**
 * @brief class for parsing .conf file using ServerConfig class as vector for multiple servers
 */
class ConfParser
{
public:
    ConfParser( const std::string& filename );
	~ConfParser();

	const std::string&					getFilename( void ) const;
	const std::vector<ServerConfig>&	getServers( void ) const;

	void						setServers( const std::vector<ServerConfig> servers );

    void						parseConfigFile( void );


private:

	std::string					_filename;
    std::vector<ServerConfig>	_servers;

	void						addServer( const ServerConfig& server );
	
	ConfParser( const ConfParser& other );
	ConfParser& operator=( const ConfParser& other );

};