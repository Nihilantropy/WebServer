#pragma once

#include "../parser/ServerConfig.hpp"

/**
 * @brief Class to set default values for ServerConfig objects
 */
class ServerConfigDefaults
{
public:
    /**
     * @brief Sets default values for a ServerConfig object
     * 
     * @param config ServerConfig object to set defaults for
     */
    static void setDefaults(ServerConfig& config);

private:
    /**
     * @brief Sets default error pages if none are defined
     * 
     * @param config ServerConfig object to set defaults for
     */
    static void _setDefaultErrorPages(ServerConfig& config);
    
    /**
     * @brief Sets default client max body size if not defined
     * 
     * @param config ServerConfig object to set defaults for
     */
    static void _setDefaultClientMaxBodySize(ServerConfig& config);
    
    /**
     * @brief Sets default server name if not defined
     * 
     * @param config ServerConfig object to set defaults for
     */
    static void _setDefaultServerName(ServerConfig& config);
};