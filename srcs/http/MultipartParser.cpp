#include "../utils/StringUtils.hpp"
#include "MultipartParser.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

MultipartParser::MultipartParser(const std::string& contentType, const std::string& body)
    : _boundary(_extractBoundary(contentType)), _body(body), _fields(), _files()
{
}

MultipartParser::~MultipartParser()
{
}

std::string MultipartParser::_extractBoundary(const std::string& contentType)
{
    // Find boundary parameter
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    // Extract boundary value
    std::string boundary = contentType.substr(boundaryPos + 9); // 9 = length of "boundary="
    
    // Remove quotes if present
    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1, boundary.length() - 2);
    }
    
    // Remove any trailing parameters
    size_t semicolonPos = boundary.find(';');
    if (semicolonPos != std::string::npos) {
        boundary = boundary.substr(0, semicolonPos);
    }
    
    // Trim whitespace
    boundary = StringUtils::trim(boundary, whiteSpaces);
    
    return boundary;
}

bool MultipartParser::parse()
{
    if (_boundary.empty()) {
        return false;
    }
    
    // Split body into parts using the boundary
    std::string delimiter = "--" + _boundary;
    std::string endDelimiter = delimiter + "--";
    
    size_t pos = _body.find(delimiter);
    if (pos == std::string::npos) {
        return false;
    }
    
    // Skip the first boundary
    pos += delimiter.length();
    
    while (pos < _body.length()) {
        // Check if this is the end boundary
        if (pos + 2 <= _body.length() && _body.substr(pos, 2) == "--") {
            break;
        }
        
        // Skip the CRLF after the boundary
        if (pos + 2 <= _body.length() && _body.substr(pos, 2) == "\r\n") {
            pos += 2;
        }
        
        // Find the next boundary
        size_t nextPos = _body.find(delimiter, pos);
        if (nextPos == std::string::npos) {
            // If no more boundaries, look for the end delimiter
            nextPos = _body.find(endDelimiter, pos);
            if (nextPos == std::string::npos) {
                // If no end delimiter, use the end of the body
                nextPos = _body.length();
            }
        }
        
        // Extract the part content
        std::string part = _body.substr(pos, nextPos - pos);
        
        // Remove trailing CRLF before the next boundary
        if (part.length() >= 2 && part.substr(part.length() - 2) == "\r\n") {
            part = part.substr(0, part.length() - 2);
        }
        
        // Parse the part
        _parsePart(part);
        
        // Move to the next part
        pos = nextPos + delimiter.length();
    }
    
    return true;
}

void MultipartParser::_parsePart(const std::string& part)
{
    // Find the dividing line between headers and body
    size_t headerEnd = part.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return;
    }
    
    // Extract headers and body
    std::string headers = part.substr(0, headerEnd);
    std::string body = part.substr(headerEnd + 4); // +4 for \r\n\r\n
    
    // Parse headers
    std::map<std::string, std::string> partHeaders = _parsePartHeaders(headers);
    
    // Check if this part has a Content-Disposition header
    std::map<std::string, std::string>::const_iterator contentDispositionIt = partHeaders.find("content-disposition");
    if (contentDispositionIt == partHeaders.end()) {
        return;
    }
    
    // Parse Content-Disposition header
    std::map<std::string, std::string> disposition = _parseContentDisposition(contentDispositionIt->second);
    
    // Check if this is a file upload or a form field
    std::map<std::string, std::string>::const_iterator filenameIt = disposition.find("filename");
    
    if (filenameIt != disposition.end() && !filenameIt->second.empty()) {
        // This is a file upload
        UploadedFile file;
        file.name = disposition["name"];
        file.filename = filenameIt->second;
        
        // Get content type if available
        std::map<std::string, std::string>::const_iterator contentTypeIt = partHeaders.find("content-type");
        if (contentTypeIt != partHeaders.end()) {
            file.contentType = contentTypeIt->second;
        } else {
            file.contentType = "application/octet-stream";
        }
        
        file.content = body;
        
        _files.push_back(file);
    } else {
        // This is a form field
        _fields[disposition["name"]] = body;
    }
}

std::map<std::string, std::string> MultipartParser::_parsePartHeaders(const std::string& headers)
{
    std::map<std::string, std::string> result;
    std::istringstream iss(headers);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Remove trailing CR if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
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
        name = StringUtils::trim(name, whiteSpaces);
        value = StringUtils::trim(value, whiteSpaces);
        
        // Convert name to lowercase for case-insensitive matching
        for (size_t i = 0; i < name.length(); ++i) {
            name[i] = tolower(name[i]);
        }
        
        result[name] = value;
    }
    
    return result;
}

std::map<std::string, std::string> MultipartParser::_parseContentDisposition(const std::string& contentDisposition)
{
    std::map<std::string, std::string> result;
    
    // Split by semicolons
    size_t start = 0;
    size_t end = contentDisposition.find(';');
    
    while (end != std::string::npos) {
        std::string part = contentDisposition.substr(start, end - start);
        
        // Trim whitespace
        part = StringUtils::trim(part, whiteSpaces);
        
        // Check if this is the form-data part
        if (part == "form-data") {
            result["type"] = "form-data";
        } else {
            // This is a name=value pair
            size_t equalPos = part.find('=');
            if (equalPos != std::string::npos) {
                std::string name = part.substr(0, equalPos);
                std::string value = part.substr(equalPos + 1);
                
                // Trim whitespace
                name = StringUtils::trim(name, whiteSpaces);
                value = StringUtils::trim(value, whiteSpaces);
                
                // Remove quotes if present
                if (!value.empty() && value[0] == '"' && value[value.length() - 1] == '"') {
                    value = value.substr(1, value.length() - 2);
                }
                
                result[name] = value;
            }
        }
        
        // Move to the next part
        start = end + 1;
        end = contentDisposition.find(';', start);
    }
    
    // Process the last part
    std::string part = contentDisposition.substr(start);
    part = StringUtils::trim(part, whiteSpaces);
    
    // Check if this is the form-data part
    if (part == "form-data") {
        result["type"] = "form-data";
    } else {
        // This is a name=value pair
        size_t equalPos = part.find('=');
        if (equalPos != std::string::npos) {
            std::string name = part.substr(0, equalPos);
            std::string value = part.substr(equalPos + 1);
            
            // Trim whitespace
            name = StringUtils::trim(name, whiteSpaces);
            value = StringUtils::trim(value, whiteSpaces);
            
            // Remove quotes if present
            if (!value.empty() && value[0] == '"' && value[value.length() - 1] == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            result[name] = value;
        }
    }
    
    return result;
}

const std::map<std::string, std::string>& MultipartParser::getFields() const
{
    return _fields;
}

const std::vector<UploadedFile>& MultipartParser::getFiles() const
{
    return _files;
}

std::string MultipartParser::getField(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it = _fields.find(name);
    if (it != _fields.end()) {
        return it->second;
    }
    return "";
}

bool MultipartParser::saveFile(size_t index, const std::string& path) const
{
    if (index >= _files.size()) {
        return false;
    }
    
    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(_files[index].content.c_str(), _files[index].content.size());
    return !file.bad();
}