# HTTP Protocol Implementation

This document provides a detailed overview of the HTTP protocol implementation in our WebServer project. It covers the key classes, their responsibilities, and how they work together to process HTTP requests and generate responses according to the HTTP/1.1 specification.

## 1. Overview of HTTP Components

Our HTTP implementation consists of several key components:

1. **Headers**: Class for managing HTTP headers
2. **Request**: Class for parsing HTTP requests
3. **Response**: Class for generating HTTP responses
4. **StatusCodes**: Defines HTTP status codes and reason phrases
5. **MultipartParser**: Parses multipart/form-data for file uploads
6. **FileUtils**: Utility functions for file operations
7. **Connection (Updated)**: Integrates HTTP processing into the server

Together, these components handle the entire HTTP request/response lifecycle, including request parsing, response generation, file serving, and error handling.

## 2. HTTP Headers Class

The `Headers` class provides a clean interface for working with HTTP headers:

### Key Features
- **Case-insensitive header names**: HTTP header names are case-insensitive by spec
- **Header normalization**: Standardizes header formatting
- **Standard header utilities**: Methods for common headers like Content-Length, Content-Type, etc.
- **Serialization**: Convert headers to properly formatted HTTP strings

### Implementation Details
- Headers are stored in a map with normalized (lowercase) names for case-insensitive lookup
- Provides methods for setting, getting, and removing headers
- Includes utility methods for parsing common headers
- Handles keep-alive detection and connection management

### Example Usage
```cpp
Headers headers;
headers.set("Content-Type", "text/html");
headers.set("Content-Length", "1024");

// Case-insensitive lookup
std::string type = headers.get("content-type");  // Returns "text/html"

// Utility methods
size_t length = headers.getContentLength();      // Returns 1024
bool keepAlive = headers.keepAlive();            // Check keep-alive
```

## 3. HTTP Request Class

The `Request` class handles parsing and representing HTTP requests:

### Key Features
- **Request line parsing**: Method, URI, and HTTP version
- **Header parsing**: Using the Headers class
- **Body parsing**: Including chunked transfer encoding
- **Query parameter parsing**: Extracting and decoding query parameters
- **Method validation**: Support for GET, POST, DELETE methods

### Implementation Details
- Implements a state machine for progressive parsing (headers → body)
- Handles different body formats based on Content-Type and Transfer-Encoding
- URL-decodes query parameters
- Provides access to all request components
- Handles reset for connection reuse

### Example Usage
```cpp
Request request;
request.parse(buffer);  // Parse from a buffer

// Access request components
Request::Method method = request.getMethod();
std::string path = request.getPath();
std::string param = request.getQueryParam("id");
const Headers& headers = request.getHeaders();
const std::string& body = request.getBody();
```

## 4. HTTP Response Class

The `Response` class generates HTTP responses:

### Key Features
- **Status code management**: Sets appropriate status codes and reason phrases
- **Header generation**: Sets common headers like Content-Type, Content-Length
- **Body handling**: Sets the response body with appropriate content type
- **Serialization**: Builds the complete HTTP response

### Implementation Details
- Uses the StatusCodes module for status code validation and reason phrases
- Sets default headers like Server and Connection
- Provides convenience methods for common response types
- Handles proper formatting of the HTTP response

### Example Usage
```cpp
Response response(HTTP_STATUS_OK);
response.setHeader("Content-Type", "text/html");
response.setBody("<html><body>Hello World</body></html>");

// Generate the complete response
std::string responseStr = response.build();
```

## 5. StatusCodes Module

The `StatusCodes` module provides definitions for HTTP status codes:

### Key Features
- **Status code constants**: Defines common HTTP status codes
- **Reason phrases**: Maps status codes to standard reason phrases
- **Code validation**: Ensures valid status codes are used

### Implementation Details
- Uses a static map for status codes to reason phrases
- Provides a utility function to get the reason phrase for a status code
- Covers all common status codes from the HTTP specification

### Example Usage
```cpp
// Use status code constants
response.setStatusCode(HTTP_STATUS_NOT_FOUND);

// Get reason phrase for a status code
std::string phrase = getReasonPhrase(404);  // Returns "Not Found"
```

## 6. MultipartParser Class

The `MultipartParser` class handles parsing multipart/form-data for file uploads:

### Key Features
- **Boundary extraction**: Parses the boundary from Content-Type header
- **Part parsing**: Extracts individual parts from multipart data
- **File extraction**: Extracts uploaded files and their metadata
- **Form field extraction**: Extracts form fields from the multipart data

### Implementation Details
- Parses the multipart format according to the HTTP specification
- Handles Content-Disposition and Content-Type headers for each part
- Extracts file content, filenames, and content types
- Provides access to parsed files and form fields

### Example Usage
```cpp
MultipartParser parser(contentType, requestBody);
if (parser.parse()) {
    const std::vector<UploadedFile>& files = parser.getFiles();
    const std::map<std::string, std::string>& fields = parser.getFields();
    
    // Process files and fields
}
```

## 7. FileUtils Class

The `FileUtils` class provides utilities for file operations:

### Key Features
- **File existence checking**: Check if a file exists
- **Directory detection**: Check if a path is a directory
- **File reading**: Read file contents
- **MIME type detection**: Get MIME type based on file extension
- **Directory listing**: Generate HTML directory listings
- **Path resolution**: Resolve request paths to file system paths

### Implementation Details
- Uses standard C/C++ file operations
- Implements MIME type mapping for common file types
- Generates formatted HTML for directory listings
- Handles path resolution based on location configuration

### Example Usage
```cpp
// Check if file exists
if (FileUtils::fileExists(path)) {
    // Serve the file
    std::string content = FileUtils::getFileContents(path);
    std::string mimeType = FileUtils::getMimeType(FileUtils::getFileExtension(path));
    response.setBody(content, mimeType);
}
```

## 8. Updated Connection Class

The `Connection` class has been updated to integrate the HTTP implementation:

### Key Features
- **HTTP request handling**: Using the Request class
- **HTTP response generation**: Using the Response class
- **File serving**: Static file serving based on request path
- **Directory handling**: Directory listing when enabled
- **Error handling**: Proper HTTP error responses
- **Method handling**: Implementation of GET, POST, DELETE methods

### Implementation Details
- Uses a more sophisticated state machine for HTTP processing
- Delegates request parsing to the Request class
- Delegates response generation to the Response class
- Implements route matching based on server configuration
- Handles file uploads using the MultipartParser class
- Serves static files with proper MIME types

### Key Request Handling Methods
- **_handleStaticFile()**: Serves static files for GET requests
- **_handlePostRequest()**: Handles POST requests (forms and uploads)
- **_handleDeleteRequest()**: Handles DELETE requests
- **_handleDirectory()**: Handles directory listings
- **_findLocation()**: Matches request paths to location configurations
- **_handleRedirection()**: Handles HTTP redirects
- **_handleError()**: Generates error responses

## 9. Processing Flow

The HTTP processing flow is as follows:

1. **Connection receives data** from client socket
2. **Request parsing**:
   - Parse headers using Request::parseHeaders()
   - Parse body using Request::parseBody() if needed
3. **Request processing**:
   - Determine the appropriate method handler
   - Match request path to location configuration
   - Execute the appropriate handler
4. **Response generation**:
   - Create a Response object
   - Set appropriate status code and headers
   - Set response body
5. **Response sending**:
   - Serialize response using Response::build()
   - Send data to client socket
6. **Connection reuse**:
   - Reset for the next request if keep-alive
   - Close if connection: close

## 10. Design Decisions and Considerations

### Modularity
We've designed the HTTP implementation with clear separation of concerns:
- **Headers**: Handles HTTP header parsing and manipulation
- **Request**: Focuses on request parsing
- **Response**: Focuses on response generation
- **Connection**: Orchestrates the HTTP processing flow

### Standards Compliance
The implementation follows HTTP/1.1 standards:
- Proper header formatting
- Status codes and reason phrases
- Support for keep-alive connections
- Chunked transfer encoding
- Content-Type and Content-Length handling

### Performance Considerations
- Non-blocking I/O for all operations
- Progressive parsing to minimize memory usage
- Reuse of buffers and objects for connection keep-alive
- Efficient path matching for request routing

### Security Considerations
- Path validation to prevent directory traversal
- Content length limits
- Method validation
- Error handling to prevent information leakage

## 11. Future Enhancements

While the current implementation provides a solid foundation, several enhancements could be made in the future:

1. **CGI Implementation**: Complete the CGI implementation for dynamic content
2. **File Upload Storage**: Implement actual file storage for uploads
3. **File Deletion**: Implement actual file deletion for DELETE requests
4. **Cookie Support**: Add support for cookies and sessions
5. **Compression**: Add support for gzip compression
6. **Caching Headers**: Implement proper caching headers
7. **Authentication**: Add basic authentication support

## 12. Conclusion

Our HTTP implementation provides a robust foundation for the WebServer project, with proper parsing, protocol compliance, and a clean API. The modular design makes it easy to extend and maintain, while the integration with the Connection class ensures proper handling of client connections and server configuration.