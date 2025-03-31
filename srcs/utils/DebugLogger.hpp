#pragma once

#include <iostream>
#include <string>
#include <sstream>

class DebugLogger {
private:
    static bool _enabled;

public:
    static void enable() { _enabled = true; }
    static void disable() { _enabled = false; }
    static bool isEnabled() { return _enabled; }
    
    // C++98 compatible toString template
    template<typename T>
    static std::string toString(const T& value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
    
    static void log(const std::string& message) {
        if (_enabled) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }
    
    template<typename T>
    static void log(const std::string& prefix, const T& value) {
        if (_enabled) {
            std::cout << "[DEBUG] " << prefix << ": " << value << std::endl;
        }
    }
    
    static void logRequest(const std::string& clientIp, const std::string& method, 
                        const std::string& path, const std::string& headers) {
        if (_enabled) {
            std::cout << "\n[REQUEST] " << clientIp << " - " << method << " " << path << std::endl;
            std::cout << "------- Headers -------" << std::endl;
            std::cout << headers << std::endl;
            std::cout << "----------------------" << std::endl;
        }
    }
    
    static void logResponse(int statusCode, const std::string& headers) {
        if (_enabled) {
            std::stringstream ss;
            ss << statusCode;
            std::cout << "\n[RESPONSE] Status: " << ss.str() << std::endl;
            std::cout << "------- Headers -------" << std::endl;
            std::cout << headers << std::endl;
            std::cout << "----------------------" << std::endl;
        }
    }
    
    static void logError(const std::string& message) {
        if (_enabled) {
            std::cerr << "[ERROR] " << message << std::endl;
        }
    }
    
    static void hexDump(const std::string& label, const std::string& data, size_t maxBytes = 100) {
        if (!_enabled || data.empty())
            return;
            
        std::cout << "[HEXDUMP] " << label << " (" << data.size() << " bytes):" << std::endl;
        
        std::stringstream ss;
        size_t bytes = data.size() > maxBytes ? maxBytes : data.size();
        
        for (size_t i = 0; i < bytes; ++i) {
            unsigned char c = static_cast<unsigned char>(data[i]);
            ss << std::hex << (c < 16 ? "0" : "") << (int)c << " ";
            
            if ((i + 1) % 16 == 0 || i == bytes - 1) {
                std::cout << ss.str() << std::endl;
                ss.str("");
            }
        }
        
        if (data.size() > maxBytes) {
            std::cout << "... " << (data.size() - maxBytes) << " more bytes" << std::endl;
        }
    }
};