#include "ServerConfigDefaults.hpp"

void ServerConfigDefaults::setDefaults(ServerConfig& config)
{
    _setDefaultErrorPages(config);
    _setDefaultClientMaxBodySize(config);
    _setDefaultServerName(config);
}

void ServerConfigDefaults::_setDefaultErrorPages(ServerConfig& config)
{
    std::map<int, std::string> errorPages = config.getErrorPages();
    
    // Define default error pages if not already defined
    if (errorPages.find(404) == errorPages.end()) {
        errorPages[404] = "/errors/404.html";
    }
    
    if (errorPages.find(500) == errorPages.end()) {
        errorPages[500] = "/errors/500.html";
    }
    
    // Add other common error pages
    if (errorPages.find(400) == errorPages.end()) {
        errorPages[400] = "/errors/400.html";
    }
    
    if (errorPages.find(403) == errorPages.end()) {
        errorPages[403] = "/errors/403.html";
    }
    
    if (errorPages.find(405) == errorPages.end()) {
        errorPages[405] = "/errors/405.html";
    }
    
    if (errorPages.find(413) == errorPages.end()) {
        errorPages[413] = "/errors/413.html";
    }
    
    config.setErrorPages(errorPages);
}

void ServerConfigDefaults::_setDefaultClientMaxBodySize(ServerConfig& config)
{
    // If client max body size is not set, set a default (1MB)
    if (config.getClientMaxBodySize() == 0) {
        config.setClientMaxBodySize(1024 * 1024); // 1MB
    }
}

void ServerConfigDefaults::_setDefaultServerName(ServerConfig& config)
{
    // If no server name is set, set a default based on host:port
    if (config.getServerNames().empty()) {
        std::vector<std::string> serverNames;
        std::string defaultName = config.getHost() + ":" + std::to_string(config.getPort());
        serverNames.push_back(defaultName);
        config.setServerNames(serverNames);
    }
}