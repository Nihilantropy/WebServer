#include "CGIHandler.hpp"
#include "../utils/StringUtils.hpp"  // Added for StringUtils::trim
#include "../utils/DebugLogger.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>  // Added for std::transform

CGIHandler::CGIHandler()
    : _scriptPath(), _requestBody(), _responseBody(), _env(),
      _cgiHeaders(), _pid(-1), _cgiExitStatus(0), _cgiExecutionError(false)
{
    _inputPipe[0] = -1;
    _inputPipe[1] = -1;
    _outputPipe[0] = -1;
    _outputPipe[1] = -1;
}

CGIHandler::~CGIHandler()
{
    _cleanup();
}

bool CGIHandler::executeCGI(const Request& request, const std::string& scriptPath, 
                           const LocationConfig& location, Response& response)
{
    _scriptPath = scriptPath;
    _requestBody = request.getBody();
    
    // Reset error tracking fields
    _cgiExecutionError = false;
    _cgiExitStatus = 0;
    
    // Extract file extension
    std::string extension = "";
    size_t dotPos = scriptPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = scriptPath.substr(dotPos);
    }
    
    // Get the appropriate interpreter based on the file extension
    std::string interpreterPath = location.getInterpreterForExtension(extension);
    
    // Check if we have a valid interpreter
    if (interpreterPath.empty()) {
        std::stringstream ss;
        ss << "No interpreter found for extension: " << extension;
        std::cerr << ss.str() << std::endl;
        DebugLogger::logError(ss.str());
        _cgiExecutionError = true;
        return false;
    }
    
    // Log the interpreter that will be used
    std::stringstream interpreterLog;
    interpreterLog << "Using interpreter: " << interpreterPath << " for extension: " << extension;
    DebugLogger::log(interpreterLog.str());
    
    // Extract PATH_INFO (part of the URL path after the script)
    std::string requestPath = request.getPath();
    std::string scriptDir = scriptPath.substr(0, scriptPath.find_last_of('/'));
    std::string scriptName = scriptPath.substr(scriptPath.find_last_of('/') + 1);
    
    // Find the location path that matched this request
    std::string locationPath = location.getPath();
    std::string pathInfo = "";
    
    // Calculate PATH_INFO (everything after the script in the URL path)
    size_t scriptPos = requestPath.find(scriptName);
    if (scriptPos != std::string::npos) {
        size_t pathInfoPos = scriptPos + scriptName.length();
        if (pathInfoPos < requestPath.length()) {
            pathInfo = requestPath.substr(pathInfoPos);
        }
    }
    
    // Setup environment variables
    _setupEnvironment(request, scriptPath, pathInfo, location);
    
    // Create pipes for communication
    if (pipe(_inputPipe) < 0 || pipe(_outputPipe) < 0) {
        std::cerr << "Failed to create pipes for CGI: " << strerror(errno) << std::endl;
        DebugLogger::logError("Failed to create pipes for CGI: " + std::string(strerror(errno)));
        _cleanup();
        _cgiExecutionError = true;
        return false;
    }
    
    // Set pipes to non-blocking mode
    int flags;
    
    // Set input read pipe to non-blocking
    flags = fcntl(_inputPipe[0], F_GETFL, 0);
    fcntl(_inputPipe[0], F_SETFL, flags | O_NONBLOCK);
    
    // Set output write pipe to non-blocking
    flags = fcntl(_outputPipe[1], F_GETFL, 0);
    fcntl(_outputPipe[1], F_SETFL, flags | O_NONBLOCK);
    
    // Execute CGI script with the appropriate interpreter
    if (!_executeCGI(interpreterPath)) {
        std::cerr << "CGI execution failed" << std::endl;
        DebugLogger::logError("CGI execution failed");
        _cleanup();
        _cgiExecutionError = true;
        return false;
    }
    
    // Write request body to CGI
    if (!_requestBody.empty() && !_writeToCGI()) {
        DebugLogger::logError("Failed to write request body to CGI");
        _cleanup();
        _cgiExecutionError = true;
        return false;
    }
    
    // Close write end of input pipe after writing
    if (_inputPipe[1] != -1) {
        close(_inputPipe[1]);
        _inputPipe[1] = -1;
    }
    
    // Read CGI output
    if (!_readFromCGI()) {
        DebugLogger::logError("Failed to read from CGI");
        _cleanup();
        _cgiExecutionError = true;
        return false;
    }
    
    // Check exit status from the CGI process
    if (_cgiExitStatus != 0) {
        std::stringstream ss;
        ss << "CGI process exited with non-zero status: " << _cgiExitStatus;
        DebugLogger::logError(ss.str());
        
        // If we don't have any content but have an error, set a more specific error
        if (_responseBody.empty()) {
            _cgiExecutionError = true;
            _cleanup();
            return false;
        }
    }
    
    // Parse CGI output to separate headers and body
    _parseCGIOutput();
    
    // If execution error and no content, don't create a response
    if (_cgiExecutionError && _responseBody.empty()) {
        DebugLogger::logError("CGI execution error with no content");
        _cleanup();
        return false;
    }
    
    // Set response based on CGI output
    // Set status code if provided by CGI, default to 200 OK
    std::map<std::string, std::string>::const_iterator statusIt = _cgiHeaders.find("status");
    if (statusIt != _cgiHeaders.end()) {
        std::istringstream iss(statusIt->second);
        int statusCode;
        iss >> statusCode;
        if (iss && statusCode >= 100 && statusCode < 600) {
            response.setStatusCode(statusCode);
        }
    } else {
        response.setStatusCode(HTTP_STATUS_OK);
    }
    
    // Set Content-Type if provided by CGI
    std::map<std::string, std::string>::const_iterator contentTypeIt = _cgiHeaders.find("content-type");
    if (contentTypeIt != _cgiHeaders.end()) {
        response.setContentType(contentTypeIt->second);
    } else {
        response.setContentType("text/html");
    }
    
    // Set response body
    response.setBody(_responseBody);
    
    // Set additional headers from CGI
    for (std::map<std::string, std::string>::const_iterator it = _cgiHeaders.begin(); 
         it != _cgiHeaders.end(); ++it) {
        // Skip special headers that we've already handled
        if (it->first != "status" && it->first != "content-type" && 
            it->first != "content-length") {
            response.setHeader(it->first, it->second);
        }
    }
    
    // Log success or failure
    if (_cgiExecutionError) {
        DebugLogger::logError("CGI execution had errors but produced output");
    } else {
        DebugLogger::log("CGI execution successful for: " + scriptPath);
    }
    
    _cleanup();
    return true;
}

void CGIHandler::_setupEnvironment(const Request& request, const std::string& scriptPath, 
                                 const std::string& pathInfo, const LocationConfig& location)
{
    // Mark the location parameter as used to avoid the unused parameter warning
    (void)location;

    // Clear previous environment
    _env.clear();
    
    // CGI/1.1 required environment variables
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_PROTOCOL"] = request.getVersion();
    _env["SERVER_SOFTWARE"] = "WebServer/1.0";
    _env["SERVER_NAME"] = request.getHost();
    
    // Request information
    _env["REQUEST_METHOD"] = request.getMethodStr();
    _env["REQUEST_URI"] = request.getUri();
    _env["PATH_INFO"] = pathInfo;
    _env["PATH_TRANSLATED"] = scriptPath;
    _env["SCRIPT_NAME"] = request.getPath();
    _env["SCRIPT_FILENAME"] = scriptPath;
    _env["QUERY_STRING"] = request.getQueryString();
    
    // Client information
    _env["REMOTE_ADDR"] = "127.0.0.1"; // This should be the client's IP
    
    // Content information
    if (request.getMethod() == Request::POST) {
        std::stringstream contentLength;
        contentLength << request.getBody().length();
        _env["CONTENT_LENGTH"] = contentLength.str();
        _env["CONTENT_TYPE"] = request.getHeaders().getContentType();
    }
    
    // Copy HTTP headers to environment variables
    const Headers& headers = request.getHeaders();
    const std::map<std::string, std::string>& allHeaders = headers.getAll();
    
    for (std::map<std::string, std::string>::const_iterator it = allHeaders.begin(); 
         it != allHeaders.end(); ++it) {
        std::string name = it->first;
        
        // Convert header name to CGI format (HTTP_HEADER_NAME)
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
        
        // Replace dashes with underscores
        for (size_t i = 0; i < name.length(); ++i) {
            if (name[i] == '-') {
                name[i] = '_';
            }
        }
        
        _env["HTTP_" + name] = it->second;
    }
    
    // CGI path information
    _env["REDIRECT_STATUS"] = "200";
    
    // CGI script directory
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        _env["DOCUMENT_ROOT"] = scriptPath.substr(0, lastSlash);
    }
}

bool CGIHandler::_executeCGI(const std::string& interpreterPath)
{
    _pid = fork();
    
    if (_pid < 0) {
        std::cerr << "Failed to fork for CGI: " << strerror(errno) << std::endl;
        DebugLogger::logError("Failed to fork for CGI: " + std::string(strerror(errno)));
        return false;
    }
    
    if (_pid == 0) {
        // Child process
        
        // Redirect stdin to input pipe
        if (dup2(_inputPipe[0], STDIN_FILENO) < 0) {
            std::cerr << "Failed to redirect stdin: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Redirect stdout to output pipe
        if (dup2(_outputPipe[1], STDOUT_FILENO) < 0) {
            std::cerr << "Failed to redirect stdout: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Close unused pipe ends
        close(_inputPipe[0]);
        close(_inputPipe[1]);
        close(_outputPipe[0]);
        close(_outputPipe[1]);
        
        // Check if script exists and is accessible
        if (access(_scriptPath.c_str(), F_OK) != 0) {
            std::cerr << "CGI script does not exist: " << _scriptPath << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Check if interpreter exists and is executable
        if (access(interpreterPath.c_str(), F_OK) != 0) {
            std::cerr << "CGI interpreter does not exist: " << interpreterPath << std::endl;
            exit(EXIT_FAILURE);
        }
        
        if (access(interpreterPath.c_str(), X_OK) != 0) {
            std::cerr << "CGI interpreter is not executable: " << interpreterPath << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Change directory to the script's directory
        std::string scriptDir = _scriptPath.substr(0, _scriptPath.find_last_of('/'));
        if (chdir(scriptDir.c_str()) < 0) {
            std::cerr << "Failed to change directory: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Prepare environment variables
        char** envp = new char*[_env.size() + 1];
        size_t i = 0;
        
        for (std::map<std::string, std::string>::const_iterator it = _env.begin(); 
             it != _env.end(); ++it) {
            std::string env = it->first + "=" + it->second;
            envp[i] = new char[env.length() + 1];
            std::strcpy(envp[i], env.c_str());
            ++i;
        }
        
        envp[i] = NULL;
        
        // Prepare command and arguments
        char* const argv[] = { 
            const_cast<char*>(interpreterPath.c_str()),   // Interpreter path
            const_cast<char*>(_scriptPath.c_str()),       // Script path
            NULL 
        };
        
        // Execute CGI script
        execve(interpreterPath.c_str(), argv, envp);
        
        // If execve returns, there was an error
        std::cerr << "Failed to execute CGI script: " << strerror(errno) << std::endl;
        
        // Clean up memory
        for (size_t j = 0; j < i; ++j) {
            delete[] envp[j];
        }
        delete[] envp;
        
        exit(EXIT_FAILURE);
    }
    
    // Parent process
    
    // Close unused pipe ends
    close(_inputPipe[0]);
    _inputPipe[0] = -1;
    
    close(_outputPipe[1]);
    _outputPipe[1] = -1;
    
    return true;
}

bool CGIHandler::_writeToCGI()
{
    if (_inputPipe[1] < 0 || _requestBody.empty()) {
        return true;
    }
    
    ssize_t bytesWritten = write(_inputPipe[1], _requestBody.c_str(), _requestBody.size());
    
    if (bytesWritten < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Try again later
            return true;
        }
        
        std::cerr << "Failed to write to CGI: " << strerror(errno) << std::endl;
        DebugLogger::logError("Failed to write to CGI: " + std::string(strerror(errno)));
        return false;
    }
    
    // In a real implementation, we might need to handle partial writes and try again
    // For simplicity, we're assuming the write completes in one go
    
    return true;
}

bool CGIHandler::_readFromCGI()
{
    if (_outputPipe[0] < 0) {
        DebugLogger::logError("Output pipe not valid");
        return false;
    }
    
    char buffer[4096];
    ssize_t bytesRead;
    bool done = false;
    bool processExited = false;
    
    // Read output until the process completes
    while (!done) {
        bytesRead = read(_outputPipe[0], buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            _responseBody += buffer;
        } else if (bytesRead == 0) {
            // End of output
            done = true;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available yet, check if the process has finished
                if (!processExited) {
                    int status;
                    int waitResult = waitpid(_pid, &status, WNOHANG);
                    
                    if (waitResult > 0) {
                        // Process has exited, store the exit status
                        processExited = true;
                        
                        if (WIFEXITED(status)) {
                            // Process exited normally, store exit code
                            _cgiExitStatus = WEXITSTATUS(status);
                            if (_cgiExitStatus != 0) {
                                // Non-zero exit code means there was an error
                                std::cerr << "CGI process exited with non-zero status: " << _cgiExitStatus << std::endl;
                                std::stringstream ss;
                                ss << "CGI process exited with status: " << _cgiExitStatus;
                                DebugLogger::logError(ss.str());
                                _cgiExecutionError = true;
                            }
                        } else if (WIFSIGNALED(status)) {
                            // Process was terminated by a signal
                            _cgiExitStatus = 128 + WTERMSIG(status);
                            std::cerr << "CGI process terminated by signal: " << WTERMSIG(status) << std::endl;
                            std::stringstream ssig;
                            ssig << "CGI process terminated by signal: " << WTERMSIG(status);
                            DebugLogger::logError(ssig.str());
                            _cgiExecutionError = true;
                        } else {
                            // Other types of termination
                            _cgiExitStatus = 1; // Generic error
                            std::cerr << "CGI process terminated abnormally" << std::endl;
                            DebugLogger::logError("CGI process terminated abnormally");
                            _cgiExecutionError = true;
                        }
                        
                        // If there's no more data to read, we're done
                        if (_responseBody.empty()) {
                            done = true;
                        }
                    } else if (waitResult < 0) {
                        // Error checking process status
                        std::cerr << "Error checking CGI process status: " << strerror(errno) << std::endl;
                        DebugLogger::logError("Error checking CGI process status: " + std::string(strerror(errno)));
                        _cgiExecutionError = true;
                        return false;
                    }
                    
                    // If we've waited long enough with no data and the process hasn't exited,
                    // consider killing it after a timeout (not implemented here)
                }
                
                // Wait a bit for more data
                usleep(1000); // 1ms
            } else {
                // Error reading from pipe
                std::cerr << "Failed to read from CGI: " << strerror(errno) << std::endl;
                DebugLogger::logError("Failed to read from CGI: " + std::string(strerror(errno)));
                _cgiExecutionError = true;
                return false;
            }
        }
    }
    
    // If we haven't already waited for the process, do it now
    if (!processExited) {
        int status;
        pid_t waitResult = waitpid(_pid, &status, 0);
        
        if (waitResult > 0) {
            if (WIFEXITED(status)) {
                _cgiExitStatus = WEXITSTATUS(status);
                if (_cgiExitStatus != 0) {
                    std::cerr << "CGI process exited with non-zero status: " << _cgiExitStatus << std::endl;
                    std::stringstream ss;
                    ss << "CGI process exited with status: " << _cgiExitStatus;
                    DebugLogger::logError(ss.str());
                    _cgiExecutionError = true;
                }
            } else if (WIFSIGNALED(status)) {
                _cgiExitStatus = 128 + WTERMSIG(status);
                std::cerr << "CGI process terminated by signal: " << WTERMSIG(status) << std::endl;
                std::stringstream ssig;
                ssig << "CGI process terminated by signal: " << WTERMSIG(status);
                DebugLogger::logError(ssig.str());
                _cgiExecutionError = true;
            } else {
                _cgiExitStatus = 1;
                std::cerr << "CGI process terminated abnormally" << std::endl;
                DebugLogger::logError("CGI process terminated abnormally");
                _cgiExecutionError = true;
            }
        } else if (waitResult < 0) {
            std::cerr << "Error waiting for CGI process: " << strerror(errno) << std::endl;
            DebugLogger::logError("Error waiting for CGI process: " + std::string(strerror(errno)));
            _cgiExecutionError = true;
            return false;
        }
    }
    
    // At this point, we should have all the output and the exit status
    return true;
}

void CGIHandler::_parseCGIOutput()
{
    // Clear previous headers
    _cgiHeaders.clear();
    
    // Find the dividing line between headers and body
    size_t headerEnd = _responseBody.find("\r\n\r\n");
    
    if (headerEnd == std::string::npos) {
        // No headers, assume entire output is body
        DebugLogger::log("No headers found in CGI output, assuming entire output is body");
        return;
    }
    
    // Extract headers section
    std::string headers = _responseBody.substr(0, headerEnd);
    
    // Remove headers from response body
    _responseBody = _responseBody.substr(headerEnd + 4);
    
    // Parse headers
    std::istringstream iss(headers);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Remove trailing CR if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        // Find the colon separator
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        
        // Extract name and value
        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // Trim whitespace
        name = StringUtils::trim(name, " \t");
        value = StringUtils::trim(value, " \t");
        
        // Convert name to lowercase for case-insensitive matching
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        // Store the header
        _cgiHeaders[name] = value;
    }
}

void CGIHandler::_cleanup()
{
    // Close pipes
    if (_inputPipe[0] != -1) {
        close(_inputPipe[0]);
        _inputPipe[0] = -1;
    }
    if (_inputPipe[1] != -1) {
        close(_inputPipe[1]);
        _inputPipe[1] = -1;
    }
    if (_outputPipe[0] != -1) {
        close(_outputPipe[0]);
        _outputPipe[0] = -1;
    }
    if (_outputPipe[1] != -1) {
        close(_outputPipe[1]);
        _outputPipe[1] = -1;
    }
    
    // Check if child process is still running
    if (_pid > 0) {
        // Try to wait for the process, but don't block
        int status;
        pid_t result = waitpid(_pid, &status, WNOHANG);
        
        if (result == 0) {
            // Process is still running, kill it
            DebugLogger::logError("CGI process still running, sending SIGTERM");
            kill(_pid, SIGTERM);
            
            // Wait a bit for it to terminate
            usleep(1000); // 1ms
            
            // Check if it's still running
            result = waitpid(_pid, &status, WNOHANG);
            if (result == 0) {
                // Still running, force kill
                DebugLogger::logError("CGI process still running after SIGTERM, sending SIGKILL");
                kill(_pid, SIGKILL);
                
                // Wait for it to terminate
                waitpid(_pid, &status, 0);
            }
        }
        
        _pid = -1;
    }
}

const std::string& CGIHandler::getResponseBody() const
{
    return _responseBody;
}

const std::map<std::string, std::string>& CGIHandler::getCGIHeaders() const
{
    return _cgiHeaders;
}

bool CGIHandler::hasExecutionError() const
{
    return _cgiExecutionError;
}

int CGIHandler::getExitStatus() const
{
    return _cgiExitStatus;
}