// srcs/tests/WebServerTests.hpp
#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "../config/parser/ConfParser.hpp"
#include "../server/Server.hpp"
#include "../server/Socket.hpp"
#include "../server/IOMultiplexer.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../http/Headers.hpp"
#include "../http/MultipartParser.hpp"
#include "../cgi/CGIHandler.hpp"
#include "../utils/FileUtils.hpp"

/**
 * @brief Comprehensive test suite for WebServer components
 */
class WebServerTests {
public:
    /**
     * @brief Run all tests
     * 
     * @return true if all tests pass, false otherwise
     */
    static bool runAllTests();

private:
    // Utility methods
    static bool createTestFile(const std::string& path, const std::string& content);
    static void cleanupTestFile(const std::string& path);
    static void printTestResult(const std::string& testName, bool success);
    static std::string makeRandomString(size_t length);
    static std::string buildMultipartContent(const std::string& boundary, 
                                            const std::map<std::string, std::string>& fields,
                                            const std::vector<std::pair<std::string, std::string> >& files);

    // HTTP Tests
    static bool testHttpHeaderParsing();
    static bool testHttpRequestParsing();
    static bool testHttpResponseGeneration();
    static bool testChunkedEncoding();
    static bool testQueryParameters();

    // Server Component Tests
    static bool testSocketCreation();
    static bool testIOMultiplexer();
    static bool testConnectionStates();
    
    // File Operation Tests
    static bool testStaticFileServing();
    static bool testDirectoryListing();
    static bool testMimeTypeDetection();
    static bool testFileUpload();
    
    // CGI Tests
    static bool testCgiEnvironmentSetup();
    static bool testCgiExecution();
    
    // Integration Tests
    static bool testGetRequest();
    static bool testPostRequest();
    static bool testDeleteRequest();
    static bool testErrorHandling();
    
    // Helper for HTTP request simulation
    static bool simulateRequest(const std::string& method, 
                               const std::string& path,
                               const std::map<std::string, std::string>& headers, 
                               const std::string& body,
                               int& statusCode,
                               std::string& responseBody);
};