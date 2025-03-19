#pragma once

#include <string>
#include <map>
#include <vector>
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../config/parser/LocationConfig.hpp"

class CGIHandler {
private:
    // Input/output for CGI
    std::string _scriptPath;
    std::string _requestBody;
    std::string _responseBody;
    
    // Environment variables
    std::map<std::string, std::string> _env;
    
    // CGI response headers
    std::map<std::string, std::string> _cgiHeaders;
    
    // Process information
    pid_t _pid;
    int _inputPipe[2];  // Server to CGI
    int _outputPipe[2]; // CGI to Server
    
    // Set up environment variables from request
    void _setupEnvironment(const Request& request, const std::string& scriptPath, 
                          const std::string& pathInfo, const LocationConfig& location);
    
    // Execute the CGI script
    bool _executeCGI();
    
    // Read output from CGI
    bool _readFromCGI();
    
    // Write input to CGI
    bool _writeToCGI();
    
    // Parse CGI output to separate headers and body
    void _parseCGIOutput();
    
    // Clean up process and pipes
    void _cleanup();

public:
    CGIHandler();
    ~CGIHandler();
    
    // Process a request through CGI
    bool executeCGI(const Request& request, const std::string& scriptPath, 
                   const LocationConfig& location, Response& response);
    
    // Get the CGI output
    const std::string& getResponseBody() const;
    
    // Get CGI response headers
    const std::map<std::string, std::string>& getCGIHeaders() const;
};