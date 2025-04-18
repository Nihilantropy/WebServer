// srcs/tests/WebServerTests.hpp
#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "../config/parser/ServerConfig.hpp"
#include "../cgi/CGIHandler.hpp"
#include "../server/Socket.hpp"
#include "../server/Connection.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../utils/FileUtils.hpp"

/**
 * @brief Simplified test suite for WebServer components
 */
class WebServerTests {
public:
    // Test directory paths
    static const std::string TEST_DIR;
    static const std::string UPLOAD_DIR;
    
    /**
     * @brief Run all tests
     * 
     * @return true if all tests pass, false otherwise
     */
    static bool runAllTests();

private:
    // Setup method
    static void setupTestDirectories();
    
    // Helper methods
    static bool createTestFile(const std::string& path, const std::string& content);
    static void cleanupTestFile(const std::string& path);
    static void printTestResult(const std::string& testName, bool success);
    
    // Test methods
    static bool testHttpCore();
    static bool testFileServing();
    static bool testDirectoryListing();
    static bool testFileUpload();
    static bool testCgiExecution();
    
    // Helper for HTTP request simulation
    static bool simulateRequest(
        const std::string& method, 
        const std::string& path,
        const std::map<std::string, std::string>& headers, 
        const std::string& body,
        int& statusCode,
        std::string& responseBody
    );
    
    // Helper for creating test directories
    static bool setupTestDir(const std::string& path);
    static void cleanupTestDir(const std::string& path);
};