Description: Unsure result on custom header in request
Test: curl -v -H "X-Custom-Header: TestValue" -H "Accept-Language: en-US" http://localhost:8080/
Result: "
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 47944 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> GET / HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> X-Custom-Header: TestValue
> Accept-Language: en-US
> 
< HTTP/1.1 200 OK
< Connection: keep-alive
< Content-Length: 21
< Content-Type: text/html
< Server: WebServer/1.0
< 
* Connection #0 to host localhost left intact
<h1>Hello World!</h1>%
"

---

Description: Unsure result on large body-size file upload
Test: curl -v -X POST -F "file=@large_file.txt" http://localhost:8080/uploads/
Result: "
Note: Unnecessary use of -X or --request, POST is already inferred.
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 49778 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> POST /uploads/ HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> Content-Length: 2097356
> Content-Type: multipart/form-data; boundary=------------------------FIU0NAsRAXKFBd9WaN5hs1
> Expect: 100-continue
> 
* Done waiting for 100-continue
* We are completely uploaded and fine
< HTTP/1.1 413 Payload Too Large
< Connection: keep-alive
< Content-Length: 151
< Content-Type: text/html
< Server: WebServer/1.0
< 
<html>
<head><title>Error 413</title></head>
<body>
  <h1>Error 413</h1>
  <p>Payload Too Large</p>
  <hr>
  <p>WebServer</p>
</body>
</html>
* Connection #0 to host localhost left intact
"