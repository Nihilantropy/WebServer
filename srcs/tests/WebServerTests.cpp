// srcs/tests/WebServerTests.cpp
#include "WebServerTests.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>

// Initialize random seed
namespace {
    bool randomInitialized = false;
    void initRandom() {
        if (!randomInitialized) {
            srand(static_cast<unsigned int>(time(NULL)));
            randomInitialized = true;
        }
    }

    // Test directory that will be used for file operations
    const std::string TEST_DIR = "/tmp/webserv_tests/";
    const std::string UPLOAD_DIR = TEST_DIR + "uploads/";
    
    // Ensure test directories exist
    void setupTestDirectories() {
        // Create test directories if they don't exist
        system(("mkdir -p " + TEST_DIR).c_str());
        system(("mkdir -p " + UPLOAD_DIR).c_str());
        
        // Set permissions
        system(("chmod 755 " + TEST_DIR).c_str());
        system(("chmod 755 " + UPLOAD_DIR).c_str());
    }
}

bool WebServerTests::runAllTests() {
    setupTestDirectories();
    
    std::cout << "\n====== RUNNING WEBSERVER TESTS ======\n" << std::endl;
    
    bool allPassed = true;
    
    // HTTP Tests
    bool httpHeadersTest = testHttpHeaderParsing();
    printTestResult("HTTP Headers Parsing", httpHeadersTest);
    allPassed &= httpHeadersTest;
    
    bool httpRequestTest = testHttpRequestParsing();
    printTestResult("HTTP Request Parsing", httpRequestTest);
    allPassed &= httpRequestTest;
    
    bool httpResponseTest = testHttpResponseGeneration();
    printTestResult("HTTP Response Generation", httpResponseTest);
    allPassed &= httpResponseTest;
    
    bool chunkedTest = testChunkedEncoding();
    printTestResult("Chunked Encoding", chunkedTest);
    allPassed &= chunkedTest;
    
    bool queryParamsTest = testQueryParameters();
    printTestResult("Query Parameters", queryParamsTest);
    allPassed &= queryParamsTest;
    
    // Server Component Tests
    bool socketTest = testSocketCreation();
    printTestResult("Socket Creation", socketTest);
    allPassed &= socketTest;
    
    bool multiplexerTest = testIOMultiplexer();
    printTestResult("IO Multiplexer", multiplexerTest);
    allPassed &= multiplexerTest;
    
    bool connectionStatesTest = testConnectionStates();
    printTestResult("Connection States", connectionStatesTest);
    allPassed &= connectionStatesTest;
    
    // File Operation Tests
    bool staticFileTest = testStaticFileServing();
    printTestResult("Static File Serving", staticFileTest);
    allPassed &= staticFileTest;
    
    bool directoryListingTest = testDirectoryListing();
    printTestResult("Directory Listing", directoryListingTest);
    allPassed &= directoryListingTest;
    
    bool mimeTypeTest = testMimeTypeDetection();
    printTestResult("MIME Type Detection", mimeTypeTest);
    allPassed &= mimeTypeTest;
    
    bool fileUploadTest = testFileUpload();
    printTestResult("File Upload", fileUploadTest);
    allPassed &= fileUploadTest;
    
    // CGI Tests
    bool cgiEnvTest = testCgiEnvironmentSetup();
    printTestResult("CGI Environment Setup", cgiEnvTest);
    allPassed &= cgiEnvTest;
    
    bool cgiExecTest = testCgiExecution();
    printTestResult("CGI Execution", cgiExecTest);
    allPassed &= cgiExecTest;
    
    // Integration Tests
    bool getTest = testGetRequest();
    printTestResult("GET Request", getTest);
    allPassed &= getTest;
    
    bool postTest = testPostRequest();
    printTestResult("POST Request", postTest);
    allPassed &= postTest;
    
    bool deleteTest = testDeleteRequest();
    printTestResult("DELETE Request", deleteTest);
    allPassed &= deleteTest;
    
    bool errorTest = testErrorHandling();
    printTestResult("Error Handling", errorTest);
    allPassed &= errorTest;
    
    std::cout << "\n====== WEBSERVER TESTS " 
              << (allPassed ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m")
              << " ======\n" << std::endl;
    
    return allPassed;
}

// Utility Methods Implementation
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

void WebServerTests::printTestResult(const std::string& testName, bool success) {
    std::cout << "Test: " << testName << " - " 
              << (success ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m") 
              << std::endl;
}

std::string WebServerTests::makeRandomString(size_t length) {
    initRandom();
    
    static const char charset[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::string result;
    result.resize(length);
    
    for (size_t i = 0; i < length; ++i) {
        result[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    
    return result;
}

std::string WebServerTests::buildMultipartContent(
    const std::string& boundary,
    const std::map<std::string, std::string>& fields,
    const std::vector<std::pair<std::string, std::string> >& files) {
    
    std::string result;
    
    // Add form fields
    for (std::map<std::string, std::string>::const_iterator it = fields.begin(); 
         it != fields.end(); ++it) {
        result += "--" + boundary + "\r\n";
        result += "Content-Disposition: form-data; name=\"" + it->first + "\"\r\n\r\n";
        result += it->second + "\r\n";
    }
    
    // Add files
    for (std::vector<std::pair<std::string, std::string> >::const_iterator it = files.begin();
         it != files.end(); ++it) {
        result += "--" + boundary + "\r\n";
        result += "Content-Disposition: form-data; name=\"file\"; filename=\"" + it->first + "\"\r\n";
        result += "Content-Type: application/octet-stream\r\n\r\n";
        result += it->second + "\r\n";
    }
    
    // Add closing boundary
    result += "--" + boundary + "--\r\n";
    
    return result;
}

// HTTP Tests Implementation
bool WebServerTests::testHttpHeaderParsing() {
    Headers headers;
    
    // Test basic header parsing
    std::string headerStr = "Content-Type: text/html\r\n"
                           "Content-Length: 1024\r\n"
                           "Connection: keep-alive\r\n";
    
    if (!headers.parse(headerStr)) {
        return false;
    }
    
    if (headers.get("content-type") != "text/html" ||
        headers.get("content-length") != "1024" ||
        headers.get("connection") != "keep-alive") {
        return false;
    }
    
    // Test case-insensitivity
    if (headers.get("Content-Type") != "text/html" ||
        headers.get("CONTENT-LENGTH") != "1024") {
        return false;
    }
    
    // Test utility methods
    if (headers.getContentLength() != 1024 ||
        headers.getContentType() != "text/html" ||
        !headers.keepAlive()) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testHttpRequestParsing() {
    // Create a simple HTTP request
    std::string requestStr = "GET /index.html?param1=value1&param2=value2 HTTP/1.1\r\n"
                            "Host: example.com\r\n"
                            "User-Agent: WebServerTest/1.0\r\n"
                            "Accept: */*\r\n"
                            "\r\n";
    
    std::string buffer = requestStr;
    Request request;
    
    // Test parsing
    if (!request.parse(buffer)) {
        return false;
    }
    
    // Verify parsed data
    if (request.getMethod() != Request::GET ||
        request.getPath() != "/index.html" ||
        request.getQueryString() != "param1=value1&param2=value2" ||
        request.getVersion() != "HTTP/1.1" ||
        request.getQueryParam("param1") != "value1" ||
        request.getQueryParam("param2") != "value2" ||
        request.getHeaders().get("host") != "example.com") {
        return false;
    }
    
    // Test POST request with body
    std::string postRequestStr = "POST /submit HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Content-Type: application/x-www-form-urlencoded\r\n"
                                "Content-Length: 23\r\n"
                                "\r\n"
                                "name=John&surname=Doe";
    
    buffer = postRequestStr;
    request.reset();
    
    if (!request.parse(buffer)) {
        return false;
    }
    
    if (request.getMethod() != Request::POST ||
        request.getPath() != "/submit" ||
        request.getBody() != "name=John&surname=Doe" ||
        request.getHeaders().getContentLength() != 23) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testHttpResponseGeneration() {
    // Test basic response
    Response response(HTTP_STATUS_OK);
    response.setBody("<html><body>Hello World</body></html>", "text/html");
    
    std::string responseStr = response.build();
    
    if (responseStr.find("HTTP/1.1 200 OK") != 0 ||
        responseStr.find("Content-Type: text/html") == std::string::npos ||
        responseStr.find("<html><body>Hello World</body></html>") == std::string::npos) {
        return false;
    }
    
    // Test redirect response
    Response redirect;
    redirect.redirect("/new-location", HTTP_STATUS_MOVED_PERMANENTLY);
    
    std::string redirectStr = redirect.build();
    
    if (redirectStr.find("HTTP/1.1 301") != 0 ||
        redirectStr.find("Location: /new-location") == std::string::npos) {
        return false;
    }
    
    // Test error response
    Response error(HTTP_STATUS_NOT_FOUND);
    error.setBody("<html><body>404 Not Found</body></html>", "text/html");
    
    std::string errorStr = error.build();
    
    if (errorStr.find("HTTP/1.1 404") != 0) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testChunkedEncoding() {
    // Create a chunked request
    std::string chunkedRequest = "POST /submit HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Transfer-Encoding: chunked\r\n"
                                "\r\n"
                                "7\r\n"
                                "Mozilla\r\n"
                                "9\r\n"
                                "Developer\r\n"
                                "7\r\n"
                                "Network\r\n"
                                "0\r\n"
                                "\r\n";
    
    std::string buffer = chunkedRequest;
    Request request;
    
    if (!request.parse(buffer)) {
        return false;
    }
    
    if (request.getBody() != "MozillaDeveloperNetwork") {
        return false;
    }
    
    return true;
}

bool WebServerTests::testQueryParameters() {
    // Test URL with multiple query parameters
    std::string requestStr = "GET /search?q=webserver&category=software&sort=relevance HTTP/1.1\r\n"
                            "Host: example.com\r\n"
                            "\r\n";
    
    std::string buffer = requestStr;
    Request request;
    
    if (!request.parse(buffer)) {
        return false;
    }
    
    if (request.getQueryParam("q") != "webserver" ||
        request.getQueryParam("category") != "software" ||
        request.getQueryParam("sort") != "relevance") {
        return false;
    }
    
    // Test URL-encoded parameters
    requestStr = "GET /search?q=web%20server&tags=c%2B%2B,networking HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "\r\n";
    
    buffer = requestStr;
    request.reset();
    
    if (!request.parse(buffer)) {
        return false;
    }
    
    if (request.getQueryParam("q") != "web server" ||
        request.getQueryParam("tags") != "c++,networking") {
        return false;
    }
    
    return true;
}

// Server Component Tests Implementation
bool WebServerTests::testSocketCreation() {
    try {
        // Test socket creation with localhost
        Socket socket("127.0.0.1", 8081);
        socket.create();
        
        // Test non-blocking mode
        socket.setNonBlocking();
        if (!socket.isNonBlocking()) {
            socket.close();
            return false;
        }
        
        // Clean up
        socket.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Socket test failed: " << e.what() << std::endl;
        return false;
    }
}

bool WebServerTests::testIOMultiplexer() {
    IOMultiplexer multiplexer;
    
    // Create a pipe for testing
    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        return false;
    }
    
    // Set non-blocking mode
    int flags1 = fcntl(pipeFds[0], F_GETFL, 0);
    fcntl(pipeFds[0], F_SETFL, flags1 | O_NONBLOCK);
    
    int flags2 = fcntl(pipeFds[1], F_GETFL, 0);
    fcntl(pipeFds[1], F_SETFL, flags2 | O_NONBLOCK);
    
    // Test adding file descriptors
    int testData = 42;
    multiplexer.addFd(pipeFds[0], IOMultiplexer::EVENT_READ, &testData);
    
    if (multiplexer.size() != 1) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }
    
    // Write to the pipe to trigger readiness
    write(pipeFds[1], "test", 4);
    
    // Test waiting for events
    int result = multiplexer.wait(100); // 100ms timeout
    
    if (result != 1 || !multiplexer.isReadReady(pipeFds[0])) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }
    
    // Test data retrieval
    if (*(int*)multiplexer.getData(pipeFds[0]) != 42) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }
    
    // Test removing file descriptors
    multiplexer.removeFd(pipeFds[0]);
    
    if (multiplexer.size() != 0) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }
    
    // Clean up
    close(pipeFds[0]);
    close(pipeFds[1]);
    
    return true;
}

bool WebServerTests::testConnectionStates() {
    // This would be better as an integration test with a real socket
    // For now, we'll do a simplified test
    
    // Create a mock server config
    ServerConfig config;
    config.setHost("127.0.0.1");
    config.setPort(8080);
    
    // Create a sockaddr_in structure
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    // Create temporary socket pair for testing
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        return false;
    }
    
    // Create connection object
    Connection connection(sockets[0], addr, &config);
    
    // Initial state should be READING_HEADERS
    if (connection.getState() != Connection::READING_HEADERS) {
        close(sockets[0]);
        close(sockets[1]);
        return false;
    }
    
    // Test shouldRead and shouldWrite methods
    if (!connection.shouldRead() || connection.shouldWrite()) {
        close(sockets[0]);
        close(sockets[1]);
        return false;
    }
    
    // Clean up
    connection.close();
    close(sockets[1]);
    
    return true;
}

// File Operation Tests Implementation
bool WebServerTests::testStaticFileServing() {
    std::string testFilePath = TEST_DIR + "test_file.html";
    std::string testContent = "<html><body>Test Content</body></html>";
    
    // Create a test file
    if (!createTestFile(testFilePath, testContent)) {
        return false;
    }
    
    // Test file exists method
    if (!FileUtils::fileExists(testFilePath)) {
        cleanupTestFile(testFilePath);
        return false;
    }
    
    // Test get file contents
    std::string content = FileUtils::getFileContents(testFilePath);
    if (content != testContent) {
        cleanupTestFile(testFilePath);
        return false;
    }
    
    // Test mime type detection
    std::string mimeType = FileUtils::getMimeType("html");
    if (mimeType != "text/html") {
        cleanupTestFile(testFilePath);
        return false;
    }
    
    // Clean up
    cleanupTestFile(testFilePath);
    
    return true;
}

bool WebServerTests::testDirectoryListing() {
    // Create test directory structure
    std::string testDirPath = TEST_DIR + "list_test/";
    system(("mkdir -p " + testDirPath).c_str());
    
    // Create some test files
    createTestFile(testDirPath + "file1.txt", "Test file 1");
    createTestFile(testDirPath + "file2.html", "<html>Test file 2</html>");
    
    // Create a subdirectory
    system(("mkdir -p " + testDirPath + "subdir").c_str());
    
    // Generate directory listing
    std::string listing = FileUtils::generateDirectoryListing(testDirPath, "/list_test/");
    
    // Check that listing contains our files
    if (listing.find("file1.txt") == std::string::npos ||
        listing.find("file2.html") == std::string::npos ||
        listing.find("subdir") == std::string::npos) {
        
        // Clean up
        system(("rm -rf " + testDirPath).c_str());
        return false;
    }
    
    // Clean up
    system(("rm -rf " + testDirPath).c_str());
    
    return true;
}

bool WebServerTests::testMimeTypeDetection() {
    // Test various file extensions
    if (FileUtils::getMimeType("html") != "text/html" ||
        FileUtils::getMimeType("css") != "text/css" ||
        FileUtils::getMimeType("js") != "text/javascript" ||
        FileUtils::getMimeType("jpg") != "image/jpeg" ||
        FileUtils::getMimeType("png") != "image/png" ||
        FileUtils::getMimeType("gif") != "image/gif" ||
        FileUtils::getMimeType("pdf") != "application/pdf") {
        return false;
    }
    
    // Test with leading dot
    if (FileUtils::getMimeType(".html") != "text/html") {
        return false;
    }
    
    // Test unknown extension
    if (FileUtils::getMimeType("xyz") != "application/octet-stream") {
        return false;
    }
    
    return true;
}

bool WebServerTests::testFileUpload() {
    // Test the upload directory preparation
    if (!FileUtils::isDirectory(UPLOAD_DIR)) {
        if (!FileUtils::createDirectory(UPLOAD_DIR)) {
            return false;
        }
    }
    
    // Create a test multipart request body
    std::string boundary = "----WebServerBoundary";
    
    std::map<std::string, std::string> fields;
    fields["description"] = "Test file upload";
    
    std::vector<std::pair<std::string, std::string> > files;
    files.push_back(std::pair<std::string, std::string>("test.txt", "This is test file content"));
    
    std::string multipartBody = buildMultipartContent(boundary, fields, files);
    
    // Create request headers
    std::string contentType = "multipart/form-data; boundary=" + boundary;
    
    // Create a MultipartParser
    MultipartParser parser(contentType, multipartBody);
    
    // Test parsing
    if (!parser.parse()) {
        return false;
    }
    
    // Verify parsed data
    const std::map<std::string, std::string>& parsedFields = parser.getFields();
    const std::vector<UploadedFile>& parsedFiles = parser.getFiles();
    
    if (parsedFields.find("description") == parsedFields.end() ||
        parsedFields.at("description") != "Test file upload" ||
        parsedFiles.empty() ||
        parsedFiles[0].filename != "test.txt" ||
        parsedFiles[0].content != "This is test file content") {
        return false;
    }
    
    // Test saving a file
    std::string savePath = UPLOAD_DIR + "uploaded_test.txt";
    if (!parser.saveFile(0, savePath)) {
        return false;
    }
    
    // Verify saved file
    std::string savedContent = FileUtils::getFileContents(savePath);
    if (savedContent != "This is test file content") {
        cleanupTestFile(savePath);
        return false;
    }
    
    // Clean up
    cleanupTestFile(savePath);
    
    return true;
}

// CGI Tests Implementation
bool WebServerTests::testCgiEnvironmentSetup() {
    // This test depends on the internal implementation of CGIHandler
    // Since we can't directly access the environment variables setup
    // we'll test the CGI output which depends on proper environment setup
    
    // Create a simple test CGI script
    std::string testCgiPath = TEST_DIR + "test.cgi";
    std::string cgiScript = 
        "#!/bin/sh\n"
        "echo \"Content-type: text/plain\\r\\n\"\n"
        "echo \"\\r\"\n"
        "echo \"HTTP_HOST=$HTTP_HOST\"\n"
        "echo \"REQUEST_METHOD=$REQUEST_METHOD\"\n"
        "echo \"QUERY_STRING=$QUERY_STRING\"\n"
        "echo \"PATH_INFO=$PATH_INFO\"\n";
    
    if (!createTestFile(testCgiPath, cgiScript)) {
        return false;
    }
    
    // Make the script executable
    if (chmod(testCgiPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        cleanupTestFile(testCgiPath);
        return false;
    }
    
    // Create a CGIHandler
    CGIHandler handler;
    
    // Create a test request
    Request request;
    std::string requestStr = 
        "GET /cgi-bin/test.cgi/extra/path?param=value HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    std::string buffer = requestStr;
    request.parse(buffer);
    
    // Create a location config
    LocationConfig location;
    location.setPath("/cgi-bin/");
    location.setRoot(TEST_DIR);
    
    std::vector<std::string> cgiExtensions;
    cgiExtensions.push_back(".cgi");
    location.setCgiExtentions(cgiExtensions);
    location.setCgiPath("/bin/sh");
    
    // Create a response object
    Response response;
    
    // Execute the CGI script
    bool result = handler.executeCGI(request, testCgiPath, location, response);
    
    // Clean up
    cleanupTestFile(testCgiPath);
    
    if (!result) {
        return false;
    }
    
    // Check the response body for environment variables
    const std::string& body = response.getBody();
    
    if (body.find("HTTP_HOST=example.com") == std::string::npos ||
        body.find("REQUEST_METHOD=GET") == std::string::npos ||
        body.find("QUERY_STRING=param=value") == std::string::npos) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testCgiExecution() {
    // Create a simple PHP test script
    std::string testPhpPath = TEST_DIR + "test.php";
    std::string phpScript = 
        "<?php\n"
        "header('Content-Type: text/html');\n"
        "echo '<html><body><h1>PHP Test</h1>';\n"
        "echo '<p>Query: ' . $_GET['test'] . '</p>';\n"
        "echo '</body></html>';\n"
        "?>";
    
    if (!createTestFile(testPhpPath, phpScript)) {
        return false;
    }
    
    // Create a CGIHandler
    CGIHandler handler;
    
    // Create a test request
    Request request;
    std::string requestStr = 
        "GET /test.php?test=hello HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    std::string buffer = requestStr;
    request.parse(buffer);
    
    // Create a location config
    LocationConfig location;
    location.setPath("/");
    location.setRoot(TEST_DIR);
    
    std::vector<std::string> cgiExtensions;
    cgiExtensions.push_back(".php");
    location.setCgiExtentions(cgiExtensions);
    
    // Use the system's PHP-CGI
    location.setCgiPath("/usr/bin/php-cgi");
    
    // Create a response object
    Response response;
    
    // Check if PHP-CGI is available
    if (access("/usr/bin/php-cgi", X_OK) != 0) {
        std::cout << "Warning: php-cgi not available, skipping PHP CGI test" << std::endl;
        cleanupTestFile(testPhpPath);
        return true; // Skip test
    }
    
    // Execute the CGI script
    bool result = handler.executeCGI(request, testPhpPath, location, response);
    
    // Clean up
    cleanupTestFile(testPhpPath);
    
    if (!result) {
        return false;
    }
    
    // Check the response
    const std::string& body = response.getBody();
    
    if (body.find("<h1>PHP Test</h1>") == std::string::npos ||
        body.find("<p>Query: hello</p>") == std::string::npos) {
        return false;
    }
    
    return true;
}

// Integration Tests Implementation - These simulate the server behavior
// with mock HTTP requests

// Helper function to simulate HTTP requests through Connection class
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

bool WebServerTests::testGetRequest() {
    // Create a test file
    std::string testFilePath = TEST_DIR + "get_test.html";
    std::string testContent = "<html><body><h1>GET Test</h1></body></html>";
    
    if (!createTestFile(testFilePath, testContent)) {
        return false;
    }
    
    // Set up request
    std::map<std::string, std::string> headers;
    headers["Host"] = "example.com";
    
    int statusCode;
    std::string responseBody;
    
    bool result = simulateRequest(
        "GET", 
        "/get_test.html", 
        headers, 
        "", 
        statusCode, 
        responseBody
    );
    
    // Clean up
    cleanupTestFile(testFilePath);
    
    if (!result) {
        return false;
    }
    
    // Verify response
    if (statusCode != 200 || responseBody != testContent) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testPostRequest() {
    // Create a test file for POST response
    std::string testFilePath = TEST_DIR + "post_result.html";
    std::string testContent = "<html><body><h1>POST Success</h1></body></html>";
    
    if (!createTestFile(testFilePath, testContent)) {
        return false;
    }
    
    // Set up request
    std::map<std::string, std::string> headers;
    headers["Host"] = "example.com";
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    
    std::string requestBody = "name=John&age=30";
    
    int statusCode;
    std::string responseBody;
    
    bool result = simulateRequest(
        "POST", 
        "/post_result.html", 
        headers, 
        requestBody, 
        statusCode, 
        responseBody
    );
    
    // Clean up
    cleanupTestFile(testFilePath);
    
    if (!result) {
        return false;
    }
    
    // Verify response
    if (statusCode != 200) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testDeleteRequest() {
    // Create a test file to delete
    std::string testFilePath = TEST_DIR + "delete_test.txt";
    
    if (!createTestFile(testFilePath, "This file will be deleted")) {
        return false;
    }
    
    // Set up request
    std::map<std::string, std::string> headers;
    headers["Host"] = "example.com";
    
    int statusCode;
    std::string responseBody;
    
    bool result = simulateRequest(
        "DELETE", 
        "/delete_test.txt", 
        headers, 
        "", 
        statusCode, 
        responseBody
    );
    
    if (!result) {
        cleanupTestFile(testFilePath);
        return false;
    }
    
    // Verify file was deleted
    bool fileExists = FileUtils::fileExists(testFilePath);
    
    // Clean up in case deletion didn't work
    if (fileExists) {
        cleanupTestFile(testFilePath);
    }
    
    // Verify response
    if (statusCode != 200 || fileExists) {
        return false;
    }
    
    return true;
}

bool WebServerTests::testErrorHandling() {
    // Create a custom 404 error page
    std::string errorPagePath = TEST_DIR + "404.html";
    std::string errorContent = "<html><body><h1>Custom 404 Error</h1></body></html>";
    
    if (!createTestFile(errorPagePath, errorContent)) {
        return false;
    }
    
    // Request a non-existent file
    std::map<std::string, std::string> headers;
    headers["Host"] = "example.com";
    
    int statusCode;
    std::string responseBody;
    
    bool result = simulateRequest(
        "GET", 
        "/non_existent_file.html", 
        headers, 
        "", 
        statusCode, 
        responseBody
    );
    
    // Clean up
    cleanupTestFile(errorPagePath);
    
    if (!result) {
        return false;
    }
    
    // Verify response
    if (statusCode != 404 || responseBody.find("Custom 404 Error") == std::string::npos) {
        return false;
    }
    
    return true;
}