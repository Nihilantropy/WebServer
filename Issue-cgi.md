Description: basic cgi path test with GET method
Test: curl -v http://localhost:8080/cgi-bin/info.php
Result: "
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 40040 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> GET /cgi-bin/info.php HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< Connection: keep-alive
< Content-Length: 0
< Content-Type: text/html
< Server: WebServer/1.0
< 
* Connection #0 to host localhost left intact
"
Server Logs: "
New connection from 127.0.0.1 (fd: 6)
[DEBUG] [DEBUG-UPLOAD] Pre-read state: 0, Input buffer size: 0
[DEBUG] Read 93 bytes, total buffer: 93
Read 93 bytes from 127.0.0.1
[HEXDUMP] Raw request data (93 bytes):
47 45 54 20 2f 63 67 69 2d 62 69 6e 2f 69 6e 66 
6f 2e 70 68 70 20 48 54 54 50 2f 31 2e 31 0d 0a 
48 6f 73 74 3a 20 6c 6f 63 61 6c 68 6f 73 74 3a 
38 30 38 30 0d 0a 55 73 65 72 2d 41 67 65 6e 74 
3a 20 63 75 72 6c 2f 38 2e 35 2e 30 0d 0a 41 63 
63 65 70 74 3a 20 2a 2f 2a 0d 0a 0d 0a 
[DEBUG] Parsing headers...
[DEBUG] Found end of headers at position 89
[DEBUG] Headers raw data:
[DEBUG] GET /cgi-bin/info.php HTTP/1.1
Host: localhost:8080
User-Agent: curl/8.5.0
Accept: */*


[DEBUG] Request line: GET /cgi-bin/info.php HTTP/1.1
[DEBUG] Method: GET, URI: /cgi-bin/info.php, Version: HTTP/1.1
[DEBUG] Path: /cgi-bin/info.php (no query string)
[DEBUG] Header line: Host: localhost:8080
[DEBUG] Header line: User-Agent: curl/8.5.0
[DEBUG] Header line: Accept: */*
[DEBUG] Found empty line, end of headers
[DEBUG] Parsed headers:
[DEBUG] accept: */*
[DEBUG] host: localhost:8080
[DEBUG] user-agent: curl/8.5.0
[DEBUG] No body expected, request is complete
[DEBUG] Headers parsed successfully
[DEBUG] Content-Length: 0, Content-Type: 

[REQUEST] 127.0.0.1 - GET /cgi-bin/info.php
------- Headers -------
Accept: */*
Host: localhost:8080
User-Agent: curl/8.5.0

----------------------
[DEBUG] Request is complete (no body required), moving to PROCESSING state
Processing GET request for /cgi-bin/info.php from 127.0.0.1
[DEBUG] Processing request: GET /cgi-bin/info.php
Request method is: 0
[DEBUG] Finding location for path: /cgi-bin/info.php
[DEBUG] Checking if location '/' matches request '/cgi-bin/info.php'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/info.php'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/info.php'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] Found location block: /cgi-bin/ with root: ./var/www/cgi
[DEBUG] Allowed methods for this location:
[DEBUG]  - GET
[DEBUG]  - POST
[DEBUG] Handling GET request
[DEBUG] Handling static file for path: /cgi-bin/info.php
[DEBUG] Finding location for path: /cgi-bin/info.php
[DEBUG] Checking if location '/' matches request '/cgi-bin/info.php'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/info.php'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/info.php'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] Resolving URI path: /cgi-bin/info.php for location path: /cgi-bin/ with root: ./var/www/cgi
[DEBUG] Converted relative root to absolute: /home/crea/Desktop/WebServer/./var/www/cgi
[DEBUG] Added trailing slash to root path: /home/crea/Desktop/WebServer/./var/www/cgi/
[DEBUG] Resolved path: /home/crea/Desktop/WebServer/./var/www/cgi/info.php
[DEBUG] Resolved filesystem path: /home/crea/Desktop/WebServer/./var/www/cgi/info.php
[DEBUG] Serving regular file: /home/crea/Desktop/WebServer/./var/www/cgi/info.php
[DEBUG] Serving file: /home/crea/Desktop/WebServer/./var/www/cgi/info.php
[DEBUG] Read file contents, size: 675
[DEBUG] File extension: php, MIME type: application/octet-stream
[DEBUG] Finding location for path: /cgi-bin/info.php
[DEBUG] Checking if location '/' matches request '/cgi-bin/info.php'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/info.php'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/info.php'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/info.php'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] File is a CGI script, extension: php
Failed to execute CGI script: Exec format error
CGI execution successful for: /home/crea/Desktop/WebServer/./var/www/cgi/info.php
[DEBUG] Building response with status code: 200
[DEBUG] Added response headers
[DEBUG] Complete response size: 110

[RESPONSE] Status: 200
------- Headers -------
Connection: keep-alive
Content-Length: 0
Content-Type: text/html
Server: WebServer/1.0

----------------------
[DEBUG] Response body length: 0
[DEBUG] Writing response data, buffer size: 110
Wrote 110 bytes to 127.0.0.1
[DEBUG] Wrote 110 bytes to client
[DEBUG] Remaining output buffer size: 0
[DEBUG] Response fully sent
[DEBUG] Keep-alive: yes
[DEBUG] Keeping connection alive, resetting for next request
[DEBUG] [DEBUG-UPLOAD] Pre-read state: 0, Input buffer size: 0
Connection closed by client: 127.0.0.1
"

---

Description: Get parameter test
Test: curl -v "http://localhost:8080/cgi-bin/get_params.php?name=John&age=30&city=New%20York"
Result: "
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 54664 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> GET /cgi-bin/get_params.php?name=John&age=30&city=New%20York HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< Connection: keep-alive
< Content-Length: 0
< Content-Type: text/html
< Server: WebServer/1.0
< 
* Connection #0 to host localhost left intact
"
Server Logs: "
New connection from 127.0.0.1 (fd: 6)
[DEBUG] [DEBUG-UPLOAD] Pre-read state: 0, Input buffer size: 0
[DEBUG] Read 132 bytes, total buffer: 132
Read 132 bytes from 127.0.0.1
[HEXDUMP] Raw request data (132 bytes):
47 45 54 20 2f 63 67 69 2d 62 69 6e 2f 67 65 74 
5f 70 61 72 61 6d 73 2e 70 68 70 3f 6e 61 6d 65 
3d 4a 6f 68 6e 26 61 67 65 3d 33 30 26 63 69 74 
79 3d 4e 65 77 25 32 30 59 6f 72 6b 20 48 54 54 
50 2f 31 2e 31 0d 0a 48 6f 73 74 3a 20 6c 6f 63 
61 6c 68 6f 73 74 3a 38 30 38 30 0d 0a 55 73 65 
72 2d 41 67 
... 32 more bytes
[DEBUG] Parsing headers...
[DEBUG] Found end of headers at position 128
[DEBUG] Headers raw data:
[DEBUG] GET /cgi-bin/get_params.php?name=John&age=30&city=New%20York HTTP/1.1
Host: localhost:8080
User-Agent: curl/8.5.0
Accept: */*


[DEBUG] Request line: GET /cgi-bin/get_params.php?name=John&age=30&city=New%20York HTTP/1.1
[DEBUG] Method: GET, URI: /cgi-bin/get_params.php?name=John&age=30&city=New%20York, Version: HTTP/1.1
[DEBUG] Path: /cgi-bin/get_params.php, Query string: name=John&age=30&city=New%20York
[DEBUG] Header line: Host: localhost:8080
[DEBUG] Header line: User-Agent: curl/8.5.0
[DEBUG] Header line: Accept: */*
[DEBUG] Found empty line, end of headers
[DEBUG] Parsed headers:
[DEBUG] accept: */*
[DEBUG] host: localhost:8080
[DEBUG] user-agent: curl/8.5.0
[DEBUG] No body expected, request is complete
[DEBUG] Headers parsed successfully
[DEBUG] Content-Length: 0, Content-Type: 

[REQUEST] 127.0.0.1 - GET /cgi-bin/get_params.php
------- Headers -------
Accept: */*
Host: localhost:8080
User-Agent: curl/8.5.0

----------------------
[DEBUG] Request is complete (no body required), moving to PROCESSING state
Processing GET request for /cgi-bin/get_params.php from 127.0.0.1
[DEBUG] Processing request: GET /cgi-bin/get_params.php
Request method is: 0
[DEBUG] Finding location for path: /cgi-bin/get_params.php
[DEBUG] Checking if location '/' matches request '/cgi-bin/get_params.php'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/get_params.php'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/get_params.php'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] Found location block: /cgi-bin/ with root: ./var/www/cgi
[DEBUG] Allowed methods for this location:
[DEBUG]  - GET
[DEBUG]  - POST
[DEBUG] Handling GET request
[DEBUG] Handling static file for path: /cgi-bin/get_params.php
[DEBUG] Finding location for path: /cgi-bin/get_params.php
[DEBUG] Checking if location '/' matches request '/cgi-bin/get_params.php'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/get_params.php'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/get_params.php'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] Resolving URI path: /cgi-bin/get_params.php for location path: /cgi-bin/ with root: ./var/www/cgi
[DEBUG] Converted relative root to absolute: /home/crea/Desktop/WebServer/./var/www/cgi
[DEBUG] Added trailing slash to root path: /home/crea/Desktop/WebServer/./var/www/cgi/
[DEBUG] Resolved path: /home/crea/Desktop/WebServer/./var/www/cgi/get_params.php
[DEBUG] Resolved filesystem path: /home/crea/Desktop/WebServer/./var/www/cgi/get_params.php
[DEBUG] Serving regular file: /home/crea/Desktop/WebServer/./var/www/cgi/get_params.php
[DEBUG] Serving file: /home/crea/Desktop/WebServer/./var/www/cgi/get_params.php
[DEBUG] Read file contents, size: 428
[DEBUG] File extension: php, MIME type: application/octet-stream
[DEBUG] Finding location for path: /cgi-bin/get_params.php
[DEBUG] Checking if location '/' matches request '/cgi-bin/get_params.php'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/get_params.php'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/get_params.php'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/get_params.php'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] File is a CGI script, extension: php
Failed to execute CGI script: Exec format error
CGI execution successful for: /home/crea/Desktop/WebServer/./var/www/cgi/get_params.php
[DEBUG] Building response with status code: 200
[DEBUG] Added response headers
[DEBUG] Complete response size: 110

[RESPONSE] Status: 200
------- Headers -------
Connection: keep-alive
Content-Length: 0
Content-Type: text/html
Server: WebServer/1.0

----------------------
[DEBUG] Response body length: 0
[DEBUG] Writing response data, buffer size: 110
Wrote 110 bytes to 127.0.0.1
[DEBUG] Wrote 110 bytes to client
[DEBUG] Remaining output buffer size: 0
[DEBUG] Response fully sent
[DEBUG] Keep-alive: yes
[DEBUG] Keeping connection alive, resetting for next request
[DEBUG] [DEBUG-UPLOAD] Pre-read state: 0, Input buffer size: 0
Connection closed by client: 127.0.0.1
"

---

Description: testing a sh script with tail -f command
Test: curl -v http://localhost:8080/cgi-bin/malus.sh
Result: "
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 54104 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> GET /cgi-bin/malus.sh HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< Connection: keep-alive
< Content-Length: 37
< Content-Type: text/html
< Server: WebServer/1.0
< 

echo "Sei stato hackerato!"

* Connection #0 to host localhost left intact
tail -f
"
Server Logs: "
New connection from 127.0.0.1 (fd: 6)
[DEBUG] [DEBUG-UPLOAD] Pre-read state: 0, Input buffer size: 0
[DEBUG] Read 93 bytes, total buffer: 93
Read 93 bytes from 127.0.0.1
[HEXDUMP] Raw request data (93 bytes):
47 45 54 20 2f 63 67 69 2d 62 69 6e 2f 6d 61 6c 
75 73 2e 73 68 20 48 54 54 50 2f 31 2e 31 0d 0a 
48 6f 73 74 3a 20 6c 6f 63 61 6c 68 6f 73 74 3a 
38 30 38 30 0d 0a 55 73 65 72 2d 41 67 65 6e 74 
3a 20 63 75 72 6c 2f 38 2e 35 2e 30 0d 0a 41 63 
63 65 70 74 3a 20 2a 2f 2a 0d 0a 0d 0a 
[DEBUG] Parsing headers...
[DEBUG] Found end of headers at position 89
[DEBUG] Headers raw data:
[DEBUG] GET /cgi-bin/malus.sh HTTP/1.1
Host: localhost:8080
User-Agent: curl/8.5.0
Accept: */*


[DEBUG] Request line: GET /cgi-bin/malus.sh HTTP/1.1
[DEBUG] Method: GET, URI: /cgi-bin/malus.sh, Version: HTTP/1.1
[DEBUG] Path: /cgi-bin/malus.sh (no query string)
[DEBUG] Header line: Host: localhost:8080
[DEBUG] Header line: User-Agent: curl/8.5.0
[DEBUG] Header line: Accept: */*
[DEBUG] Found empty line, end of headers
[DEBUG] Parsed headers:
[DEBUG] accept: */*
[DEBUG] host: localhost:8080
[DEBUG] user-agent: curl/8.5.0
[DEBUG] No body expected, request is complete
[DEBUG] Headers parsed successfully
[DEBUG] Content-Length: 0, Content-Type: 

[REQUEST] 127.0.0.1 - GET /cgi-bin/malus.sh
------- Headers -------
Accept: */*
Host: localhost:8080
User-Agent: curl/8.5.0

----------------------
[DEBUG] Request is complete (no body required), moving to PROCESSING state
Processing GET request for /cgi-bin/malus.sh from 127.0.0.1
[DEBUG] Processing request: GET /cgi-bin/malus.sh
Request method is: 0
[DEBUG] Finding location for path: /cgi-bin/malus.sh
[DEBUG] Checking if location '/' matches request '/cgi-bin/malus.sh'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/malus.sh'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/malus.sh'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] Found location block: /cgi-bin/ with root: ./var/www/cgi
[DEBUG] Allowed methods for this location:
[DEBUG]  - GET
[DEBUG]  - POST
[DEBUG] Handling GET request
[DEBUG] Handling static file for path: /cgi-bin/malus.sh
[DEBUG] Finding location for path: /cgi-bin/malus.sh
[DEBUG] Checking if location '/' matches request '/cgi-bin/malus.sh'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/malus.sh'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/malus.sh'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] Resolving URI path: /cgi-bin/malus.sh for location path: /cgi-bin/ with root: ./var/www/cgi
[DEBUG] Converted relative root to absolute: /home/crea/Desktop/WebServer/./var/www/cgi
[DEBUG] Added trailing slash to root path: /home/crea/Desktop/WebServer/./var/www/cgi/
[DEBUG] Resolved path: /home/crea/Desktop/WebServer/./var/www/cgi/malus.sh
[DEBUG] Resolved filesystem path: /home/crea/Desktop/WebServer/./var/www/cgi/malus.sh
[DEBUG] Serving regular file: /home/crea/Desktop/WebServer/./var/www/cgi/malus.sh
[DEBUG] Serving file: /home/crea/Desktop/WebServer/./var/www/cgi/malus.sh
[DEBUG] Read file contents, size: 47
[DEBUG] File extension: sh, MIME type: application/octet-stream
[DEBUG] Finding location for path: /cgi-bin/malus.sh
[DEBUG] Checking if location '/' matches request '/cgi-bin/malus.sh'
[DEBUG] Root location '/' is fallback match
[DEBUG] Checking if location '/cgi-bin/' matches request '/cgi-bin/malus.sh'
[DEBUG] New best match: '/cgi-bin/' (length: 9)
[DEBUG] Checking if location '/upload' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/uploads/' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/uploads/images' matches request '/cgi-bin/malus.sh'
[DEBUG] Checking if location '/kapouet/' matches request '/cgi-bin/malus.sh'
[DEBUG] Final best match location: /cgi-bin/
[DEBUG] File is a CGI script, extension: sh
CGI execution successful for: /home/crea/Desktop/WebServer/./var/www/cgi/malus.sh
[DEBUG] Building response with status code: 200
[DEBUG] Added response headers
[DEBUG] Added response body, size: 37
[DEBUG] Complete response size: 148

[RESPONSE] Status: 200
------- Headers -------
Connection: keep-alive
Content-Length: 37
Content-Type: text/html
Server: WebServer/1.0

----------------------
[DEBUG] Response body length: 37
[DEBUG] Writing response data, buffer size: 148
Wrote 148 bytes to 127.0.0.1
[DEBUG] Wrote 148 bytes to client
[DEBUG] Remaining output buffer size: 0
[DEBUG] Response fully sent
[DEBUG] Keep-alive: yes
[DEBUG] Keeping connection alive, resetting for next request
[DEBUG] [DEBUG-UPLOAD] Pre-read state: 0, Input buffer size: 0
Connection closed by client: 127.0.0.1
"
