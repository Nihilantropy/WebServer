#include "../utils/StringUtils.hpp"
#include "Headers.hpp"
#include <algorithm>
#include <cstring>
#include <cctype>

Headers::Headers() {}

Headers::~Headers() {}

std::string Headers::_normalize(const std::string& name) const
{
    std::string result = name;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void Headers::set(const std::string& name, const std::string& value)
{
    _headers[_normalize(name)] = value;
}

bool Headers::contains(const std::string& name) const
{
    return _headers.find(_normalize(name)) != _headers.end();
}

std::string Headers::get(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(_normalize(name));
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

void Headers::remove(const std::string& name)
{
    _headers.erase(_normalize(name));
}

const std::map<std::string, std::string>& Headers::getAll() const
{
    return _headers;
}

std::string Headers::toString() const
{
    std::string result;
    
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        // Capitalize first letter of each header word for standard formatting
        std::string name = it->first;
        bool capitalize = true;
        
        for (size_t i = 0; i < name.length(); ++i) {
            if (capitalize) {
                name[i] = toupper(name[i]);
                capitalize = false;
            } else if (name[i] == '-') {
                capitalize = true;
            }
        }
        
        result += name + ": " + it->second + "\r\n";
    }
    
    return result;
}

bool Headers::parse(const std::string& str)
{
    std::istringstream stream(str);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove trailing CR if present (for CRLF)
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
            return false; // Invalid header format
        }
        
        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // Trim whitespace
        name = StringUtils::trim(name, whiteSpaces);
        value = StringUtils::trim(value, whiteSpaces);
        
        set(name, value);
    }
    
    return true;
}

void Headers::clear()
{
    _headers.clear();
}

size_t Headers::getContentLength() const
{
    std::string value = get("content-length");
    if (value.empty()) {
        return 0;
    }
    
    // Convert to integer
    char* endptr;
    long result = strtol(value.c_str(), &endptr, 10);
    
    // Check if conversion was successful
    if (*endptr != '\0' || result < 0) {
        return 0;
    }
    
    return static_cast<size_t>(result);
}

std::string Headers::getContentType() const
{
    return get("content-type");
}

bool Headers::hasChunkedEncoding() const
{
    std::string transferEncoding = get("transfer-encoding");
    return transferEncoding.find("chunked") != std::string::npos;
}

bool Headers::keepAlive(bool defaultValue) const
{
    std::string connection = get("connection");
    
    if (connection.empty()) {
        return defaultValue;
    }
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
    
    if (connection.find("close") != std::string::npos) {
        return false;
    }
    
    if (connection.find("keep-alive") != std::string::npos) {
        return true;
    }
    
    return defaultValue;
}