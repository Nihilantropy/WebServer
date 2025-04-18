#include "ServerConfigValidator.hpp"
#include <sstream>

ServerConfigValidator::ServerConfigValidator(const ServerConfig& serverConfig) : _serverConfig(serverConfig)
{
    _validate();
}

ServerConfigValidator::~ServerConfigValidator() {}

void ServerConfigValidator::_validate(void) const
{
    // Validate host and port
    _validateHostAndPort();
    
    // Validate server names (optional but should be handled properly)
    _validateServerNames();
    
    // Validate client max body size
    _validateClientMaxBodySize();
    
    // Validate error pages
    _validateErrorPages();
    
    // Validate locations
    _validateLocations();
}

void ServerConfigValidator::_validateHostAndPort(void) const
{
    // Host validation
    const std::string& host = _serverConfig.getHost();
    if (host.empty()) {
        throw ValidationException("Server host cannot be empty");
    }
    
    // Simple IP validation (basic check)
    if (host != "localhost" && host != "0.0.0.0" && host != "127.0.0.1") {
        // Check if it's a valid IP format (simple check)
        size_t dotCount = 0;
        for (size_t i = 0; i < host.length(); i++) {
            if (host[i] == '.') {
                dotCount++;
            } else if (!isdigit(host[i])) {
                throw ValidationException("Invalid host: " + host);
            }
        }
        if (dotCount != 3) {
            throw ValidationException("Invalid IP format: " + host);
        }
    }
    
    // Port validation
    int port = _serverConfig.getPort();
    if (port <= 0 || port > 65535) {
        std::stringstream ss;
        ss << "Invalid port number: " << port;
        throw ValidationException(ss.str());
    }
}

void ServerConfigValidator::_validateServerNames(void) const
{
    // Server names are optional but should be valid if provided
    const std::vector<std::string>& serverNames = _serverConfig.getServerNames();
    (void)serverNames; // To avoid unused variable warning
    
    // No specific validation needed for server names, they can be any string
    // But if we want to enforce some naming conventions, we could add checks here
}

void ServerConfigValidator::_validateClientMaxBodySize(void) const
{
    const size_t& clientMaxBodySize = _serverConfig.getClientMaxBodySize();
    
    // If client max body size is not set, it should be set to a default value
    // This could be done in a "normalize" method rather than validation
    // For validation purposes, we just ensure it's not unreasonably large
    
    // 1GB is probably a reasonable upper limit for a web server
    const size_t MAX_REASONABLE_SIZE = 1024 * 1024 * 1024;
    
    if (clientMaxBodySize > MAX_REASONABLE_SIZE) {
        std::stringstream ss;
        ss << "Client max body size too large: " << clientMaxBodySize << " bytes";
        throw ValidationException(ss.str());
    }
}

void ServerConfigValidator::_validateErrorPages(void) const
{
    const std::map<int, std::string>& errorPages = _serverConfig.getErrorPages();
    
    for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it) {
        // Validate error code (should be between 300 and 599)
        if (it->first < 300 || it->first > 599) {
            std::stringstream ss;
            ss << "Invalid HTTP error code: " << it->first;
            throw ValidationException(ss.str());
        }
        
        // Validate error page path (should not be empty)
        if (it->second.empty()) {
            std::stringstream ss;
            ss << "Error page path cannot be empty for error code: " << it->first;
            throw ValidationException(ss.str());
        }
        
        // Check if file exists - might not do this during validation
        // as the file might be created later or be a relative path
        // access(it->second.c_str(), F_OK) != -1
    }
}

void ServerConfigValidator::_validateLocations(void) const
{
    const std::vector<LocationConfig*>& locations = _serverConfig.getLocations();
    
    if (locations.empty()) {
        throw ValidationException("Server must have at least one location block");
    }
    
    // Check for duplicate paths
    std::set<std::string> paths;
    
    for (std::vector<LocationConfig*>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        const std::string& path = (*it)->getPath();
        
        // Check for duplicate paths
        if (paths.find(path) != paths.end()) {
            throw ValidationException("Duplicate location path: " + path);
        }
        
        paths.insert(path);
        
        // Use the dedicated LocationConfigValidator
        LocationConfigValidator validator(**it);
    }
}