#pragma once

#include "AValidator.hpp"
#include "../parser/ServerConfig.hpp"
#include <iostream>

/**
 * @brief class to validate Server configuratio info
 * 
 * @param ServerConfig object at construction
 * 
 * @throw ValidationException subclass of runtime_exception
 */
class ServerConfigValidator : public AValidator
{
public:
	ServerConfigValidator( const ServerConfig& serverConfig );
	~ServerConfigValidator();

private:
	const ServerConfig&	_serverConfig;

	void	_validate( void ) const;
};