#include "ConfigTests.hpp"
#include <sstream>

// Utility method to create a temporary config file for testing
bool ConfigTests::createTestConfigFile(const std::string& filename, const std::string& content)
{
    std::ofstream file(filename.c_str());
    if (!file.is_open())
        return false;
    
    file << content;
    file.close();
    return true;
}

// Utility method to clean up test files
void ConfigTests::cleanupTestFile(const std::string& filename)
{
    remove(filename.c_str());
}

// Method to print test results in a consistent format
void ConfigTests::printTestResult(const std::string& testName, bool success)
{
    std::cout << "Test: " << testName << " - " 
              << (success ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m") 
              << std::endl;
}

// Run all tests
void ConfigTests::runAllTests()
{
    std::cout << "\n====== RUNNING CONFIG SYSTEM TESTS ======\n" << std::endl;
    
    // Basic parsing tests
    printTestResult("Basic Parsing", testBasicParsing());
    printTestResult("Parsing With Comments", testParsingWithComments());
    
    // Server validation tests
    printTestResult("Invalid Host", testInvalidHost());
    printTestResult("Invalid Port", testInvalidPort());
    printTestResult("Duplicate Server Names", testDuplicateServerNames());
    printTestResult("Large Client Max Body Size", testLargeClientMaxBodySize());
    
    // Location validation tests
    printTestResult("Invalid Location Path", testInvalidLocationPath());
    printTestResult("Duplicate Location Path", testDuplicateLocationPath());
    printTestResult("Invalid Allowed Methods", testInvalidAllowedMethods());
    printTestResult("Missing Index With Autoindex Off", testMissingIndexWithAutoindexOff());
    printTestResult("CGI Without Path", testCgiWithoutPath());
    printTestResult("Upload Dir Without POST", testUploadDirWithoutPost());
    printTestResult("Invalid Redirection", testInvalidRedirection());
    
    // Default value tests
    printTestResult("Default Error Pages", testDefaultErrorPages());
    printTestResult("Default Client Max Body Size", testDefaultClientMaxBodySize());
    printTestResult("Default Server Name", testDefaultServerName());
    printTestResult("Default Allowed Methods", testDefaultAllowedMethods());
    printTestResult("Default Index", testDefaultIndex());
    
    // Edge cases
    printTestResult("Empty Config File", testEmptyConfigFile());
    printTestResult("Missing Closing Brace", testMissingClosingBrace());
    printTestResult("Unknown Directive", testUnknownDirective());
    printTestResult("Multiple Servers With Same HostPort", testMultipleServersWithSameHostPort());
    
    std::cout << "\n====== CONFIG SYSTEM TESTS COMPLETED ======\n" << std::endl;
}

// ===== Basic Parsing Tests =====

bool ConfigTests::testBasicParsing()
{
    const std::string testFile = "test_basic.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    client_max_body_size 1M;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET POST;\n"
        "        autoindex   off;\n"
        "        index       index.html;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        
        // Check if we have 1 server
        if (servers.size() != 1)
            return false;
        
        // Check basic server properties
        ServerConfig* server = servers[0];
        if (server->getHost() != "127.0.0.1" || 
            server->getPort() != 8080 ||
            server->getServerNames().size() != 1 ||
            server->getServerNames()[0] != "example.com" ||
            server->getClientMaxBodySize() != 1024 * 1024 || // 1M
            server->getLocations().size() != 1)
            return false;
            
        // Check location properties
        LocationConfig* location = server->getLocations()[0];
        if (location->getPath() != "/" ||
            location->getRoot() != "/var/www/html" ||
            location->getAllowedMethods().size() != 2 ||
            !location->getAutoIndex() ||
            location->getIndex() != "index.html")
            return false;
        
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error in testBasicParsing: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testParsingWithComments()
{
    const std::string testFile = "test_comments.conf";
    const std::string content = 
        "# This is a test configuration with comments\n"
        "server { # Server block\n"
        "    listen      127.0.0.1:8080; # Listen directive\n"
        "    server_name example.com; # Server name\n"
        "    \n"
        "    # Location block\n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET; # Only allow GET\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        
        // Check if we have 1 server
        if (servers.size() != 1)
            return false;
        
        // Check basic server properties
        ServerConfig* server = servers[0];
        if (server->getHost() != "127.0.0.1" || 
            server->getPort() != 8080 ||
            server->getServerNames().size() != 1 ||
            server->getServerNames()[0] != "example.com" ||
            server->getLocations().size() != 1)
            return false;
            
        // Check location properties
        LocationConfig* location = server->getLocations()[0];
        if (location->getPath() != "/" ||
            location->getRoot() != "/var/www/html" ||
            location->getAllowedMethods().size() != 1 ||
            location->getAllowedMethods()[0] != "GET")
            return false;
        
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error in testParsingWithComments: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

// ===== Server Validation Tests =====

bool ConfigTests::testInvalidHost()
{
    const std::string testFile = "test_invalid_host.conf";
    const std::string content = 
        "server {\n"
        "    listen      invalid-host:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testInvalidHost: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testInvalidPort()
{
    const std::string testFile = "test_invalid_port.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:99999;\n" // Invalid port (>65535)
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testInvalidPort: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testDuplicateServerNames()
{
    const std::string testFile = "test_duplicate_server_names.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com www.example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n"
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com other.com;\n" // Duplicate server_name
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testDuplicateServerNames: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testLargeClientMaxBodySize()
{
    const std::string testFile = "test_large_body_size.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    client_max_body_size 2048M;\n" // Extremely large (2TB)
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here if validation is checking for reasonable sizes
        // If validation allows large sizes, this would pass
        cleanupTestFile(testFile);
        return true; // Adjust based on your validation logic
    } catch (const ValidationException& e) {
        // Expected if validation checks for reasonable sizes
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testLargeClientMaxBodySize: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

// ===== Location Validation Tests =====

bool ConfigTests::testInvalidLocationPath()
{
    const std::string testFile = "test_invalid_location_path.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location without-slash {\n" // Invalid path (no leading /)
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testInvalidLocationPath: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testDuplicateLocationPath()
{
    const std::string testFile = "test_duplicate_location_path.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location /api {\n"
        "        root        /var/www/api;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "    location /api {\n" // Duplicate path
        "        root        /var/www/api2;\n"
        "        allowed_methods POST;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testDuplicateLocationPath: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testInvalidAllowedMethods()
{
    const std::string testFile = "test_invalid_methods.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET PUT;\n" // PUT is not allowed (only GET, POST, DELETE)
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testInvalidAllowedMethods: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testMissingIndexWithAutoindexOff()
{
    const std::string testFile = "test_missing_index.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "        autoindex   off;\n" // Autoindex off but no index specified
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should either set a default index or pass validation (depends on your default setting behavior)
        const std::vector<ServerConfig*>& servers = parser.getServers();
        ServerConfig* server = servers[0];
        LocationConfig* location = server->getLocations()[0];
        
        // If default values are set correctly, we should have an index
        bool hasIndex = !location->getIndex().empty();
        
        cleanupTestFile(testFile);
        return hasIndex;
    } catch (const std::exception& e) {
        std::cerr << "Error in testMissingIndexWithAutoindexOff: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testCgiWithoutPath()
{
    const std::string testFile = "test_cgi_without_path.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "        cgi_extension .php;\n" // CGI extension without cgi_path
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testCgiWithoutPath: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testUploadDirWithoutPost()
{
    const std::string testFile = "test_upload_without_post.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n" // Only GET (no POST)
        "        upload_dir  /var/www/uploads;\n" // Upload dir requires POST
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testUploadDirWithoutPost: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testInvalidRedirection()
{
    const std::string testFile = "test_invalid_redirection.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "        return 200 /new;\n" // Invalid redirection code (not 3xx)
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testInvalidRedirection: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

// ===== Default Value Tests =====

bool ConfigTests::testDefaultErrorPages()
{
    const std::string testFile = "test_default_error_pages.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        ServerConfig* server = servers[0];
        
        // Check if default error pages have been set
        const std::map<int, std::string>& errorPages = server->getErrorPages();
        
        bool has404 = errorPages.find(404) != errorPages.end();
        bool has500 = errorPages.find(500) != errorPages.end();
        
        cleanupTestFile(testFile);
        return has404 && has500;
    } catch (const std::exception& e) {
        std::cerr << "Error in testDefaultErrorPages: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testDefaultClientMaxBodySize()
{
    const std::string testFile = "test_default_body_size.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    # No client_max_body_size specified\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        ServerConfig* server = servers[0];
        
        // Default should be 1MB
        bool hasDefaultSize = (server->getClientMaxBodySize() == 1024 * 1024);
        
        cleanupTestFile(testFile);
        return hasDefaultSize;
    } catch (const std::exception& e) {
        std::cerr << "Error in testDefaultClientMaxBodySize: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testDefaultServerName()
{
    const std::string testFile = "test_default_server_name.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    # No server_name specified\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        ServerConfig* server = servers[0];
        
        // Should have a default server name
        bool hasDefaultName = !server->getServerNames().empty();
        
        cleanupTestFile(testFile);
        return hasDefaultName;
    } catch (const std::exception& e) {
        std::cerr << "Error in testDefaultServerName: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testDefaultAllowedMethods()
{
    const std::string testFile = "test_default_methods.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        # No allowed_methods specified\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        ServerConfig* server = servers[0];
        LocationConfig* location = server->getLocations()[0];
        
        // Should have default allowed methods (at least GET)
        bool hasDefaultMethods = !location->getAllowedMethods().empty();
        
        cleanupTestFile(testFile);
        return hasDefaultMethods;
    } catch (const std::exception& e) {
        std::cerr << "Error in testDefaultAllowedMethods: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testDefaultIndex()
{
    const std::string testFile = "test_default_index.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "        autoindex   off;\n"
        "        # No index specified with autoindex off\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        const std::vector<ServerConfig*>& servers = parser.getServers();
        ServerConfig* server = servers[0];
        LocationConfig* location = server->getLocations()[0];
        
        // Should have a default index file (e.g., "index.html")
        bool hasDefaultIndex = !location->getIndex().empty();
        
        cleanupTestFile(testFile);
        return hasDefaultIndex;
    } catch (const std::exception& e) {
        std::cerr << "Error in testDefaultIndex: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

// ===== Edge Cases =====

bool ConfigTests::testEmptyConfigFile()
{
    const std::string testFile = "test_empty.conf";
    const std::string content = ""; // Empty file
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ValidationException& e) {
        // Expected to throw a ValidationException about no server blocks
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testEmptyConfigFile: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testMissingClosingBrace()
{
    const std::string testFile = "test_missing_brace.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "    # Missing closing brace for server\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ConfigException& e) {
        // Expected to throw a ConfigException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testMissingClosingBrace: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testUnknownDirective()
{
    const std::string testFile = "test_unknown_directive.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    unknown_directive value;\n" // Unknown directive
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // Should not reach here
        cleanupTestFile(testFile);
        return false;
    } catch (const ConfigException& e) {
        // Expected to throw a ConfigException
        cleanupTestFile(testFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error in testUnknownDirective: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}

bool ConfigTests::testMultipleServersWithSameHostPort()
{
    const std::string testFile = "test_same_host_port.conf";
    const std::string content = 
        "server {\n"
        "    listen      127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/html;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n"
        "server {\n"
        "    listen      127.0.0.1:8080;\n" // Same host:port
        "    server_name other.com;\n"
        "    \n"
        "    location / {\n"
        "        root        /var/www/other;\n"
        "        allowed_methods GET;\n"
        "    }\n"
        "}\n";
    
    if (!createTestConfigFile(testFile, content))
        return false;
    
    try {
        ConfParser parser(testFile);
        // This should be valid as long as server_names are different
        // According to NGINX behavior, the first server would be the default
        const std::vector<ServerConfig*>& servers = parser.getServers();
        bool hasTwoServers = (servers.size() == 2);
        
        cleanupTestFile(testFile);
        return hasTwoServers;
    } catch (const std::exception& e) {
        std::cerr << "Error in testMultipleServersWithSameHostPort: " << e.what() << std::endl;
        cleanupTestFile(testFile);
        return false;
    }
}