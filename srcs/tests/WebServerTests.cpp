// srcs/tests/WebServerTests.cpp
#include "WebServerTests.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Test directories as static class members
const std::string WebServerTests::TEST_DIR = "/tmp/webserv_tests/";
const std::string WebServerTests::UPLOAD_DIR = WebServerTests::TEST_DIR + "uploads/";

bool WebServerTests::runAllTests() {
    setupTestDirectories();
    
    std::cout << "\n====== RUNNING WEBSERVER TESTS ======\n" << std::endl;
    
    bool allPassed = true;
    
    // Core HTTP functionality
    bool httpTest = testHttpCore();
    printTestResult("HTTP Core", httpTest);
    allPassed &= httpTest;
    
    // File serving 
    bool fileTest = testFileServing();
    printTestResult("File Serving", fileTest);
    allPassed &= fileTest;
    
    // Directory listing
    bool dirListTest = testDirectoryListing();
    printTestResult("Directory Listing", dirListTest);
    allPassed &= dirListTest;
    
    // File upload
    bool uploadTest = testFileUpload();
    printTestResult("File Upload", uploadTest);
    allPassed &= uploadTest;
    
    // CGI execution
    bool cgiTest = testCgiExecution();
    printTestResult("CGI Execution", cgiTest);
    allPassed &= cgiTest;
    
    std::cout << "\n====== WEBSERVER TESTS " 
              << (allPassed ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m")
              << " ======\n" << std::endl;
    
    return allPassed;
}

    // Ensure test directories exist
void WebServerTests::setupTestDirectories() {
    system(("mkdir -p " + TEST_DIR).c_str());
    system(("mkdir -p " + UPLOAD_DIR).c_str());
    system(("chmod 755 " + TEST_DIR).c_str());
    system(("chmod 755 " + UPLOAD_DIR).c_str());
}

// Helper Methods Implementation
bool WebServerTests::createTestFile(const std::string& path, const std::string& content) {
    std::ofstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    file.close();
    return true;
}

void WebServerTests::cleanupTestFile(const std::string& path) {
    remove(path.c_str());
}

bool WebServerTests::setupTestDir(const std::string& path) {
    return system(("mkdir -p " + path).c_str()) == 0;
}

void WebServerTests::cleanupTestDir(const std::string& path) {
    system(("rm -rf " + path).c_str());
}

void WebServerTests::printTestResult(const std::string& testName, bool success) {
    std::cout << "Test: " << testName << " - " 
              << (success ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m") 
              << std::endl;
}

// Test simulation helper
bool WebServerTests::simulateRequest(
    const std::string& method, 
    const std::string& path,
    const std::map<std::string, std::string>& headers, 
    const std::string& body,
    int& statusCode,
    std::string& responseBody) {
    
    // Build HTTP request
    std::stringstream requestStream;
    requestStream << method << " " << path << " HTTP/1.1\r\n";
    
    // Add headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        requestStream << it->first << ": " << it->second << "\r\n";
    }
    
    // Add content length if body is not empty
    if (!body.empty() && headers.find("Content-Length") == headers.end()) {
        requestStream << "Content-Length: " << body.size() << "\r\n";
    }
    
    // End headers
    requestStream << "\r\n";
    
    // Add body if present
    if (!body.empty()) {
        requestStream << body;
    }
    
    std::string requestStr = requestStream.str();
    
    // Create socket pair for testing
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        return false;
    }
    
    // Write request to one end of socket pair
    if (write(sockets[1], requestStr.c_str(), requestStr.size()) != (ssize_t)requestStr.size()) {
        close(sockets[0]);
        close(sockets[1]);
        return false;
    }
    
    // Set up server configuration
    ServerConfig config;
    config.setHost("127.0.0.1");
    config.setPort(8080);
    
    // Set up error pages
    std::map<int, std::string> errorPages;
    errorPages[404] = TEST_DIR + "404.html";
    errorPages[500] = TEST_DIR + "500.html";
    config.setErrorPages(errorPages);
    
    // Set up location
    LocationConfig* location = new LocationConfig();
    location->setPath("/");
    location->setRoot(TEST_DIR);
    
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    methods.push_back("DELETE");
    location->setAllowedMethods(methods);
    
    std::vector<LocationConfig*> locations;
    locations.push_back(location);
    config.setLocations(locations);
    
    // Create a sockaddr_in structure
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    // Create connection object
    Connection connection(sockets[0], addr, &config);
    
    // Process the request
    connection.readData();
    connection.process();
    connection.writeData();
    
    // Read response
    char buffer[4096];
    ssize_t bytesRead = read(sockets[1], buffer, sizeof(buffer) - 1);
    
    // Clean up
    connection.close();
    close(sockets[1]);
    
    if (bytesRead <= 0) {
        return false;
    }
    
    buffer[bytesRead] = '\0';
    std::string response(buffer, bytesRead);
    
    // Parse status code
    size_t statusStart = response.find("HTTP/1.1 ");
    if (statusStart == std::string::npos) {
        return false;
    }
    
    statusStart += 9; // Length of "HTTP/1.1 "
    statusCode = atoi(response.substr(statusStart, 3).c_str());
    
    // Extract response body
    size_t bodyStart = response.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        responseBody = response.substr(bodyStart + 4);
    } else {
        responseBody = "";
    }
    
    return true;
}

// Test Method Implementations
bool WebServerTests::testHttpCore() {
    std::cout << "  Testing HTTP request/response handling..." << std::endl;
    
    // Test GET request
    {
        std::string testFilePath = TEST_DIR + "test.html";
        std::string testContent = "<html><body><h1>Test Page</h1></body></html>";
        
        if (!createTestFile(testFilePath, testContent)) {
            std::cerr << "  Failed to create test file" << std::endl;
            return false;
        }
        
        std::map<std::string, std::string> headers;
        headers["Host"] = "example.com";
        
        int statusCode;
        std::string responseBody;
        
        bool result = simulateRequest("GET", "/test.html", headers, "", statusCode, responseBody);
        
        cleanupTestFile(testFilePath);
        
        if (!result || statusCode != 200 || responseBody != testContent) {
            std::cerr << "  GET request test failed" << std::endl;
            return false;
        }
    }
    
    // Test POST request
    {
        std::map<std::string, std::string> headers;
        headers["Host"] = "example.com";
        headers["Content-Type"] = "application/x-www-form-urlencoded";
        
        std::string requestBody = "name=John&age=30";
        
        int statusCode;
        std::string responseBody;
        
        bool result = simulateRequest("POST", "/form", headers, requestBody, statusCode, responseBody);
        
        // For now, we're just checking that the server accepts the POST
        // In a real server, you'd check for a specific response
        if (!result || statusCode >= 400) {
            std::cerr << "  POST request test failed: " << statusCode << std::endl;
            return false;
        }
    }
    
    // Test 404 response
    {
        std::map<std::string, std::string> headers;
        headers["Host"] = "example.com";
        
        // Create a custom 404 page
        std::string errorPagePath = TEST_DIR + "404.html";
        std::string errorContent = "<html><body><h1>404 Not Found</h1></body></html>";
        
        if (!createTestFile(errorPagePath, errorContent)) {
            std::cerr << "  Failed to create error page" << std::endl;
            return false;
        }
        
        int statusCode;
        std::string responseBody;
        
        bool result = simulateRequest("GET", "/nonexistent.html", headers, "", statusCode, responseBody);
        
        cleanupTestFile(errorPagePath);
        
        if (!result || statusCode != 404) {
            std::cerr << "  404 response test failed" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool WebServerTests::testFileServing() {
    std::cout << "  Testing file serving capabilities..." << std::endl;
    
    // 1. Test serving a regular HTML file
    {
        std::string testFilePath = TEST_DIR + "index.html";
        std::string testContent = "<html><body><h1>Index Page</h1></body></html>";
        
        if (!createTestFile(testFilePath, testContent)) {
            std::cerr << "  Failed to create test file" << std::endl;
            return false;
        }
        
        std::map<std::string, std::string> headers;
        headers["Host"] = "example.com";
        
        int statusCode;
        std::string responseBody;
        
        bool result = simulateRequest("GET", "/index.html", headers, "", statusCode, responseBody);
        
        cleanupTestFile(testFilePath);
        
        if (!result || statusCode != 200 || responseBody != testContent) {
            std::cerr << "  HTML file serving test failed" << std::endl;
            return false;
        }
    }
    
    // 2. Test directory with index file
    {
        std::string testDirPath = TEST_DIR + "dir/";
        setupTestDir(testDirPath);
        
        std::string indexContent = "<html><body><h1>Directory Index</h1></body></html>";
        createTestFile(testDirPath + "index.html", indexContent);
        
        std::map<std::string, std::string> headers;
        headers["Host"] = "example.com";
        
        int statusCode;
        std::string responseBody;
        
        bool result = simulateRequest("GET", "/dir/", headers, "", statusCode, responseBody);
        
        cleanupTestDir(testDirPath);
        
        if (!result || statusCode != 200 || responseBody != indexContent) {
            std::cerr << "  Directory index file test failed" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool WebServerTests::testDirectoryListing() {
    std::cout << "  Testing directory listing..." << std::endl;
    
    // Create test directory with files
    std::string testDirPath = TEST_DIR + "listing/";
    setupTestDir(testDirPath);
    
    // Create some files in the directory
    createTestFile(testDirPath + "file1.txt", "File 1 content");
    createTestFile(testDirPath + "file2.html", "<html>File 2</html>");
    setupTestDir(testDirPath + "subdir");
    
    // Set up a LocationConfig with autoindex enabled
    LocationConfig location;
    location.setPath("/listing/");
    location.setRoot(TEST_DIR);
    location.setAutoIndex(true);
    
    // Generate a directory listing
    std::string listing = FileUtils::generateDirectoryListing(testDirPath, "/listing/");
    
    // Verify the listing contains our files
    bool success = (listing.find("file1.txt") != std::string::npos &&
                  listing.find("file2.html") != std::string::npos &&
                  listing.find("subdir") != std::string::npos);
    
    // Clean up
    cleanupTestDir(testDirPath);
    
    return success;
}

bool WebServerTests::testFileUpload() {
    std::cout << "  Testing file upload functionality..." << std::endl;
    
    // Since file upload testing is complex through the Connection simulation,
    // we'll test the MultipartParser directly here
    
    // Create multipart form data
    std::string boundary = "----WebKitFormBoundaryABC123";
    std::string body = 
        "------WebKitFormBoundaryABC123\r\n"
        "Content-Disposition: form-data; name=\"description\"\r\n"
        "\r\n"
        "Test file upload\r\n"
        "------WebKitFormBoundaryABC123\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "This is test file content\r\n"
        "------WebKitFormBoundaryABC123--\r\n";
    
    std::string contentType = "multipart/form-data; boundary=----WebKitFormBoundaryABC123";
    
    // Create parser and parse
    MultipartParser parser(contentType, body);
    bool result = parser.parse();
    
    if (!result) {
        std::cerr << "  Multipart parsing failed" << std::endl;
        return false;
    }
    
    // Check if fields and files are correctly parsed
    const std::map<std::string, std::string>& fields = parser.getFields();
    const std::vector<UploadedFile>& files = parser.getFiles();
    
    if (fields.find("description") == fields.end() || 
        fields.at("description") != "Test file upload" ||
        files.empty() || 
        files[0].filename != "test.txt" ||
        files[0].content != "This is test file content") {
        std::cerr << "  Incorrect multipart parsing result" << std::endl;
        return false;
    }
    
    // Test file saving capability
    std::string savePath = UPLOAD_DIR + "test_upload.txt";
    bool saveResult = parser.saveFile(0, savePath);
    
    if (!saveResult || !FileUtils::fileExists(savePath)) {
        std::cerr << "  File save failed" << std::endl;
        return false;
    }
    
    // Verify file content
    std::string savedContent = FileUtils::getFileContents(savePath);
    bool contentMatch = (savedContent == "This is test file content");
    
    // Clean up
    cleanupTestFile(savePath);
    
    return contentMatch;
}

bool WebServerTests::testCgiExecution() {
    std::cout << "  Testing CGI execution..." << std::endl;
    
    // Create a simple test CGI script
    std::string testScriptPath = TEST_DIR + "test.cgi";
    std::string scriptContent = 
        "#!/bin/sh\n"
        "echo \"Content-type: text/html\"\n"
        "echo \"\"\n"
        "echo \"<html><body>\"\n"
        "echo \"<h1>CGI Test</h1>\"\n"
        "echo \"<p>Query string: $QUERY_STRING</p>\"\n"
        "echo \"<p>Remote address: $REMOTE_ADDR</p>\"\n"
        "echo \"</body></html>\"\n";
    
    if (!createTestFile(testScriptPath, scriptContent)) {
        std::cerr << "  Failed to create CGI script" << std::endl;
        return false;
    }
    
    // Make the script executable
    chmod(testScriptPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    
    // Create a Request object
    Request request;
    std::string requestStr = 
        "GET /test.cgi?param=value HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    std::string buffer = requestStr;
    request.parse(buffer);
    
    // Set up a LocationConfig
    LocationConfig location;
    location.setPath("/");
    location.setRoot(TEST_DIR);
    
    std::vector<std::string> cgiExtensions;
    cgiExtensions.push_back(".cgi");
    location.setCgiExtentions(cgiExtensions);
    location.setCgiPath("/bin/sh");
    
    // Create a Response
    Response response;
    
    // Execute CGI
    CGIHandler handler;
    bool result = handler.executeCGI(request, testScriptPath, location, response);
    
    // Clean up
    cleanupTestFile(testScriptPath);
    
    if (!result) {
        std::cerr << "  CGI execution failed" << std::endl;
        return false;
    }
    
    // Check response
    const std::string& body = response.getBody();
    
    if (body.find("<h1>CGI Test</h1>") == std::string::npos ||
        body.find("Query string: param=value") == std::string::npos) {
        std::cerr << "  CGI output verification failed" << std::endl;
        return false;
    }
    
    return true;
}