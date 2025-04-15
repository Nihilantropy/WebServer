Issue: Redirect is not working
Test: curl -v http://localhost:8081/old
Result: "
* Host localhost:8081 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8081...
* connect to ::1 port 8081 from ::1 port 48274 failed: Connection refused
*   Trying 127.0.0.1:8081...
* Connected to localhost (127.0.0.1) port 8081
> GET /old HTTP/1.1
> Host: localhost:8081
> User-Agent: curl/8.5.0
> Accept: */*
> 
* Unsupported HTTP/1 subversion in response
* Closing connection
curl: (1) Unsupported HTTP/1 subversion in response
"

SOLVED

---

Issue: File upload not working
Test: curl -v -X POST -F "file=@test_upload.txt" http://localhost:8080/uploads/
Result: "
Note: Unnecessary use of -X or --request, POST is already inferred.
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 41650 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> POST /uploads/ HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> Content-Length: 225
> Content-Type: multipart/form-data; boundary=------------------------cbQbNNpW04XTGMHc6qZkQY
> 
* We are completely uploaded and fine
< HTTP/1.1 500 Internal Server Error
< Connection: keep-alive
< Content-Length: 356
< Content-Type: text/html
< Server: WebServer/1.0
< 
<html>
<head><title>Upload Result</title></head>
<body>
  <h1>Upload Result</h1>
  <p>Received 1 file(s).</p>
  <ul>
    <li>
      <strong>Error:</strong> Security violation for file: test_upload.txt<br>
    </li>
  </ul>
  <p><strong>Upload Summary:</strong> 0 of 1 files were uploaded successfully to ./var/www/uploads/</p>
</body>
</html>
* Connection #0 to host localhost left intact
"

SOLVED

---

Description: Unkown method causes infinite loop on the server
Test: curl -v -X PUT http://localhost:8080/index.html
Result: "
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 33108 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> PUT /index.html HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
> 
^C (interrupted by SIGINT)
"

---
