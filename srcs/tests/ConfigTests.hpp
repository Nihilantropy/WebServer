#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../config/parser/ConfParser.hpp"
#include "../exceptions/exceptions.hpp"

class ConfigTests
{
public:
    static void runAllTests();

private:
    // Utilities
    static bool createTestConfigFile(const std::string& filename, const std::string& content);
    static void cleanupTestFile(const std::string& filename);
    static void printTestResult(const std::string& testName, bool success);

    // Basic parsing tests
    static bool testBasicParsing();
    static bool testParsingWithComments();
    
    // Server validation tests
    static bool testInvalidHost();
    static bool testInvalidPort();
    static bool testDuplicateServerNames();
    static bool testLargeClientMaxBodySize();
    
    // Location validation tests
    static bool testInvalidLocationPath();
    static bool testDuplicateLocationPath();
    static bool testInvalidAllowedMethods();
    static bool testMissingIndexWithAutoindexOff();
    static bool testCgiWithoutPath();
    static bool testUploadDirWithoutPost();
    static bool testInvalidRedirection();
    
    // Default value tests
    static bool testDefaultErrorPages();
    static bool testDefaultClientMaxBodySize();
    static bool testDefaultServerName();
    static bool testDefaultAllowedMethods();
    static bool testDefaultIndex();
    
    // Edge cases
    static bool testEmptyConfigFile();
    static bool testMissingClosingBrace();
    static bool testUnknownDirective();
    static bool testMultipleServersWithSameHostPort();
};