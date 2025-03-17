#pragma once

#include "AValidator.hpp"
#include "../parser/ServerConfig.hpp"
#include <iostream>
#include <set>
#include <string>

/**
 * @brief class to validate Server configuration info
 * 
 * @param ServerConfig object at construction
 * 
 * @throw ValidationException subclass of runtime_exception
 */
class ServerConfigValidator : public AValidator
{
public:
	ServerConfigValidator(const ServerConfig& serverConfig);
	~ServerConfigValidator();

private:
	const ServerConfig& _serverConfig;

	void _validate(void) const;
	void _validateHostAndPort(void) const;
	void _validateServerNames(void) const;
	void _validateClientMaxBodySize(void) const;
	void _validateErrorPages(void) const;
	void _validateLocations(void) const;
};