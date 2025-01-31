#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cerrno>
#include <fstream>
#include <sstream>
#include "../utils/utils.hpp"
#include "../exceptions/exceptions.hpp"

#include "ServerConfig.hpp"

class ServerConfig;

/**
 * @brief class for parsing .conf file using ServerConfig class as vector for multiple servers.
 * 
 * @param filename the filename to parse, including the path.
 * 
 * It will try to open the file at creation.
 * 
 * @throw ConfigException if the open fail.
 */
class ConfParser
{
public:
    ConfParser( const std::string& filename );
	~ConfParser();

	const std::string&					getFilename( void ) const;
	const std::vector<ServerConfig*>&	getServers( void ) const;

	void	setFilename( const std::string& filename );
	void	setServers( const std::vector<ServerConfig*>& servers );

    void	parseConfigFile( void );

private:

	std::string					_filename;
	std::ifstream				_configFile;
    std::vector<ServerConfig*>	_servers;

	void	_openFile( void );
	void	_addServer( ServerConfig* server );
	
	ConfParser( const ConfParser& other );
	ConfParser& operator=( const ConfParser& other );

};

std::ostream&	operator<<( std::ostream& os, const ConfParser& parser );