# HTTP Protocol Implementation Design

## 1. Class Structure

We'll implement the HTTP protocol with the following class hierarchy:

```
http/
├── Headers.hpp       # HTTP Headers container and manipulation
├── Headers.cpp
├── Request.hpp       # HTTP Request parsing and representation
├── Request.cpp
├── Response.hpp      # HTTP Response generation
├── Response.cpp
├── StatusCodes.hpp   # HTTP Status codes and reason phrases
└── Utils.hpp         # HTTP utility functions
```

## 2. Class Responsibilities

### 2.1 Headers Class

The `Headers` class will be responsible for:
- Storing and retrieving HTTP headers
- Case-insensitive header lookup
- Header validation and normalization
- Common header utility methods

```cpp
class Headers {
private:
    std::map<std::string, std::string> _headers;  // Normalized header name to value

public:
    Headers();
    ~Headers();

    void set(const std::string& name, const std::string& value);
    bool contains(const std::string& name) const;
    std::string get(const std::string& name) const;
    void remove(const std::string& name);
    std::string toString() const;
    
    // Utility methods for common headers
    size_t getContentLength() const;
    std::string getContentType() const;
    bool hasChunkedEncoding() const;
    bool keepAlive() const;
};
```

### 2.2 Request Class

The `Request` class will be responsible for:
- Parsing the HTTP request line, headers, and body
- Representing the HTTP request data
- Handling query parameters
- Supporting different HTTP methods

```cpp
class Request {
public:
    enum Method {
        GET,
        POST,
        DELETE,
        UNKNOWN
    };

private:
    Method _method;               // HTTP method
    std::string _uri;             // Request URI
    std::string _path;            // URI path component
    std::string _queryString;     // URI query string
    std::map<std::string, std::string> _queryParams; // Parsed query parameters
    std::string _version;         // HTTP version
    Headers _headers;             // HTTP headers
    std::string _body;            // Request body
    bool _complete;               // Whether the request is complete
    
    // Parsing state
    bool _headersParsed;          // Whether headers have been parsed
    size_t _bodyBytesRead;        // Number of body bytes read so far
    
    // Parse components
    bool _parseRequestLine(const std::string& line);
    void _parseQueryParams();

public:
    Request();
    ~Request();

    // Parsing methods
    bool parse(std::string& buffer);
    bool parseHeaders(std::string& buffer);
    bool parseBody(std::string& buffer);
    
    // Getters
    Method getMethod() const;
    std::string getMethodStr() const;
    const std::string& getUri() const;
    const std::string& getPath() const;
    const std::string& getQueryString() const;
    std::string getQueryParam(const std::string& name) const;
    const std::map<std::string, std::string>& getQueryParams() const;
    const std::string& getVersion() const;
    const Headers& getHeaders() const;
    Headers& getHeaders();
    const std::string& getBody() const;
    bool isComplete() const;
    
    // Utility methods
    std::string toString() const;
    static Method parseMethod(const std::string& method);
    static std::string methodToString(Method method);
};
```

### 2.3 Response Class

The `Response` class will be responsible for:
- Generating HTTP response status lines, headers, and body
- Handling different status codes
- Building the complete response to send back to the client

```cpp
class Response {
private:
    int _statusCode;              // HTTP status code
    std::string _statusMessage;   // HTTP status message
    std::string _version;         // HTTP version
    Headers _headers;             // HTTP headers
    std::string _body;            // Response body
    bool _sent;                   // Whether the response has been sent

public:
    Response();
    Response(int statusCode);
    ~Response();

    // Setters
    void setStatusCode(int statusCode);
    void setVersion(const std::string& version);
    void setBody(const std::string& body, const std::string& contentType = "text/html");
    
    // Headers methods
    void setHeader(const std::string& name, const std::string& value);
    Headers& getHeaders();
    
    // Building methods
    std::string build();
    
    // Common response helpers
    void redirect(const std::string& location, int code = 302);
    void setContentType(const std::string& contentType);
    void setContentLength(size_t length);
    
    // Getters
    int getStatusCode() const;
    const std::string& getStatusMessage() const;
    const std::string& getVersion() const;
    const std::string& getBody() const;
    bool isSent() const;
    
    // Utility method
    static std::string getStatusMessage(int statusCode);
};
```

### 2.4 StatusCodes.hpp

A simple header file containing HTTP status code constants and their reason phrases:

```cpp
// HTTP status codes
#define HTTP_STATUS_OK                    200
#define HTTP_STATUS_CREATED               201
#define HTTP_STATUS_ACCEPTED              202
#define HTTP_STATUS_NO_CONTENT            204

#define HTTP_STATUS_MOVED_PERMANENTLY     301
#define HTTP_STATUS_FOUND                 302
#define HTTP_STATUS_SEE_OTHER             303
#define HTTP_STATUS_NOT_MODIFIED          304
#define HTTP_STATUS_TEMPORARY_REDIRECT    307
#define HTTP_STATUS_PERMANENT_REDIRECT    308

#define HTTP_STATUS_BAD_REQUEST           400
#define HTTP_STATUS_UNAUTHORIZED          401
#define HTTP_STATUS_FORBIDDEN             403
#define HTTP_STATUS_NOT_FOUND             404
#define HTTP_STATUS_METHOD_NOT_ALLOWED    405
#define HTTP_STATUS_REQUEST_TIMEOUT       408
#define HTTP_STATUS_LENGTH_REQUIRED       411
#define HTTP_STATUS_PAYLOAD_TOO_LARGE     413

#define HTTP_STATUS_INTERNAL_SERVER_ERROR 500
#define HTTP_STATUS_NOT_IMPLEMENTED       501
#define HTTP_STATUS_BAD_GATEWAY           502
#define HTTP_STATUS_SERVICE_UNAVAILABLE   503
#define HTTP_STATUS_GATEWAY_TIMEOUT       504
#define HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED 505

// Function to get the reason phrase for a status code
std::string getReasonPhrase(int statusCode);
```

## 3. Integration with Connection Class

The Connection class will be updated to use these HTTP classes:

```cpp
class Connection {
private:
    // Existing members...
    
    // New members for HTTP handling
    Request _request;
    Response _response;
    
    // Updated methods
    bool _parseHttpHeaders() {
        // Use Request class for parsing
        return _request.parseHeaders(_inputBuffer);
    }
    
    bool _parseHttpBody() {
        // Use Request class for parsing body
        return _request.parseBody(_inputBuffer);
    }
    
    void _processRequest() {
        // Process the request and generate response
        // This will be expanded based on the URL path, method, etc.
        
        // Create a response
        _response = Response(HTTP_STATUS_OK);
        _response.setBody("<h1>Hello from WebServer!</h1>\n");
    }
    
    void _generateResponse() {
        // Get the response as a string
        _outputBuffer = _response.build();
    }

public:
    // Existing methods...
};
```

## 4. Implementation Strategy

1. **Headers Class**: Implement first since both Request and Response depend on it.
2. **StatusCodes**: Define common HTTP status codes and reason phrases.
3. **Request Class**: Implement HTTP request parsing.
4. **Response Class**: Implement HTTP response generation.
5. **Connection Integration**: Update the Connection class to use the new HTTP classes.

## 5. Handling Different Request Methods

### GET Method
- Parse query parameters
- No request body processing
- Response typically returns content

### POST Method
- Parse form data or JSON in the request body
- Content-Type header determines how to parse the body
- Content-Length required

### DELETE Method
- Similar to GET but with different semantics
- Used for resource deletion
- May or may not have a request body

## 6. Key Implementation Details

### 6.1 Header Field Normalization

HTTP header field names are case-insensitive. We'll normalize them by converting to lowercase for storage and lookup.

### 6.2 Transfer Encoding

Support for chunked transfer encoding:
- For requests: need to reassemble chunks
- For responses: might support sending chunked responses

### 6.3 Connection Management

Based on the Connection header:
- If "close", connection closes after the response
- If "keep-alive", connection stays open (HTTP/1.1 default)

### 6.4 Status Codes

Implement common status codes:
- 2xx (Success)
- 3xx (Redirection)
- 4xx (Client Error)
- 5xx (Server Error)

### 6.5 Error Pages

Support for custom error pages based on configuration:
- Use the error pages configured in ServerConfig
- Fall back to default error pages if not configured