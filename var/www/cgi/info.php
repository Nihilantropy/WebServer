<?php
echo "Content-type: text/html\r\n\r\n";
echo "<!DOCTYPE html>\n";
echo "<html><head><title>PHP Info</title></head>\n";
echo "<body>\n";
echo "<h1>PHP CGI Test</h1>\n";
echo "<h2>Request Information:</h2>\n";
echo "<ul>\n";
echo "<li>Server Name: " . ($_SERVER['SERVER_NAME'] ?? 'Unknown') . "</li>\n";
echo "<li>Request Method: " . ($_SERVER['REQUEST_METHOD'] ?? 'Unknown') . "</li>\n";
echo "<li>Script Name: " . ($_SERVER['SCRIPT_NAME'] ?? 'Unknown') . "</li>\n";
echo "<li>Query String: " . ($_SERVER['QUERY_STRING'] ?? 'None') . "</li>\n";
echo "<li>Remote Address: " . ($_SERVER['REMOTE_ADDR'] ?? 'Unknown') . "</li>\n";
echo "</ul>\n";
echo "</body></html>\n";
?>