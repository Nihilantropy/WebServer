#include "LocationConfigValidator.hpp"

LocationConfigValidator::LocationConfigValidator(const LocationConfig& locationConfig) : _locationConfig(locationConfig)
{
    _validate();
}

LocationConfigValidator::~LocationConfigValidator() {}

void LocationConfigValidator::_validate(void) const
{
    _validatePath();
    _validateRoot();
    _validateAllowedMethods();
    _validateIndex();
    _validateCgi();
    _validateUploadDir();
    _validateRedirection();
}

void LocationConfigValidator::_validatePath(void) const
{
    const std::string& path = _locationConfig.getPath();
    
    // Path should not be empty
    if (path.empty()) {
        throw ValidationException("Location path cannot be empty");
    }
    
    // Path should start with /
    if (path[0] != '/') {
        throw ValidationException("Location path must start with /: " + path);
    }
}

void LocationConfigValidator::_validateRoot(void) const
{
    const std::string& root = _locationConfig.getRoot();
    const std::string& redirection = _locationConfig.getRedirection();
    
    // Root should not be empty UNLESS this is a redirect location
    if (root.empty() && redirection.empty()) {
        throw ValidationException("Root directory cannot be empty for location: " + _locationConfig.getPath());
    }
}

void LocationConfigValidator::_validateAllowedMethods(void) const
{
    const std::vector<std::string>& methods = _locationConfig.getAllowedMethods();
    
    // Methods should not be empty
    if (methods.empty()) {
        throw ValidationException("No HTTP methods allowed for location: " + _locationConfig.getPath());
    }
    
    // Validate each method
    for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
        if (*it != "GET" && *it != "POST" && *it != "DELETE") {
            throw ValidationException("Invalid HTTP method: " + *it + " for location: " + _locationConfig.getPath());
        }
    }
}

void LocationConfigValidator::_validateIndex(void) const
{
    // If autoindex is off, index should be provided
    if (!_locationConfig.getAutoIndex() && _locationConfig.getIndex().empty()) {
        throw ValidationException("Index file must be specified when autoindex is off for location: " + _locationConfig.getPath());
    }
}

void LocationConfigValidator::_validateCgi(void) const
{
    const std::vector<std::string>& cgiExtensions = _locationConfig.getCgiExtentions();
    const std::string& cgiPath = _locationConfig.getCgiPath();
    const std::map<std::string, std::string>& cgiHandlers = _locationConfig.getCgiHandlers();
    
    // Legacy validation
    if (!cgiExtensions.empty() && cgiPath.empty() && cgiHandlers.empty()) {
        throw ValidationException("CGI path must be specified when CGI extensions are defined for location: " + _locationConfig.getPath());
    }
    
    // Validate each CGI extension
    for (std::vector<std::string>::const_iterator it = cgiExtensions.begin(); it != cgiExtensions.end(); ++it) {
        // Extension should start with a dot
        if ((*it)[0] != '.') {
            throw ValidationException("CGI extension must start with a dot: " + *it);
        }
    }
    
    // Validate cgi_handler mappings
    for (std::map<std::string, std::string>::const_iterator it = cgiHandlers.begin(); it != cgiHandlers.end(); ++it) {
        // Extension should start with a dot
        if (it->first.empty() || it->first[0] != '.') {
            throw ValidationException("CGI handler extension must start with a dot: " + it->first);
        }
        
        // Interpreter path should not be empty
        if (it->second.empty()) {
            throw ValidationException("CGI handler interpreter path cannot be empty for extension: " + it->first);
        }
    }
}

void LocationConfigValidator::_validateUploadDir(void) const
{
    // If upload directory is specified, it should exist
    const std::string& uploadDir = _locationConfig.getUploadDir();
    if (!uploadDir.empty()) {
        // Check if POST is allowed
        bool postAllowed = false;
        const std::vector<std::string>& methods = _locationConfig.getAllowedMethods();
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == "POST") {
                postAllowed = true;
                break;
            }
        }
        
        if (!postAllowed) {
            throw ValidationException("Upload directory specified but POST method not allowed for location: " + _locationConfig.getPath());
        }
    }
}

void LocationConfigValidator::_validateRedirection(void) const
{
    // If redirection is specified, it should be valid
    const std::string& redirection = _locationConfig.getRedirection();
    if (!redirection.empty()) {
        // Parse redirection string (format should be "STATUS URL")
        std::istringstream iss(redirection);
        int statusCode = 0;
        std::string redirectUrl;
        
        // Get status code
        if (iss >> statusCode) {
            // Get redirect URL
            if (iss >> redirectUrl) {
                // Validate status code (should be 3xx)
                if (statusCode < 300 || statusCode > 399) {
                    std::stringstream ss;
                    ss << "Invalid redirect status code: " << statusCode << " for location: " << _locationConfig.getPath();
                    throw ValidationException(ss.str());
                }
                
                // Validate redirect URL
                if (redirectUrl.empty()) {
                    throw ValidationException("Redirect URL cannot be empty for location: " + _locationConfig.getPath());
                }
            } else {
                throw ValidationException("Missing redirect URL for location: " + _locationConfig.getPath());
            }
        } else {
            throw ValidationException("Invalid redirect format (should be 'STATUS URL') for location: " + _locationConfig.getPath());
        }
    }
}