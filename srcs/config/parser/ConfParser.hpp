#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "../../utils/utils.hpp"
#include "../../exceptions/exceptions.hpp"
#include "../validate/ServerConfigValidator.hpp"
#include "../validate/ServerConfigDefaults.hpp"
#include "../validate/LocationConfigDefaults.hpp"

#include "ServerConfig.hpp"

class ServerConfig;

/**
 * @brief class for parsing .conf file using ServerConfig class as vector for multiple servers.
 * 
 * @param filename the filename to parse, including the path.
 * 
 * It will try to open and parse the file at creation.
 * 
 * @throw OpenException if the open or parsing fail.
 * @throw ConfigException if the parsing fail.
 * @throw ValidationException if validation fails
 */
class ConfParser
{
public:
    ConfParser(const std::string& filename);
	~ConfParser();

	const std::string&					getFilename(void) const;
	const std::vector<ServerConfig*>&	getServers(void) const;

	void	setFilename(const std::string& filename);
	void	setServers(const std::vector<ServerConfig*>& servers);

private:
	std::string					_filename;
	std::ifstream				_configFile;
    std::vector<ServerConfig*>	_servers;

	void	_openFile(void);
	void	_addServer(ServerConfig* server);
	void	_parseConfigFile(void);
	void	_setDefaults(void);
	void	_validate(void);
	
	ConfParser(const ConfParser& other);
	ConfParser& operator=(const ConfParser& other);
};

std::ostream&	operator<<(std::ostream& os, const ConfParser& parser);