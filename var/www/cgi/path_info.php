<?php
echo "Content-type: text/html\r\n\r\n";
echo "<!DOCTYPE html>\n";
echo "<html><head><title>PATH_INFO Test</title></head>\n";
echo "<body>\n";
echo "<h1>PATH_INFO Test</h1>\n";
echo "<p><strong>PATH_INFO:</strong> " . htmlspecialchars($_SERVER['PATH_INFO'] ?? 'None') . "</p>\n";
echo "<p><strong>SCRIPT_NAME:</strong> " . htmlspecialchars($_SERVER['SCRIPT_NAME'] ?? 'Unknown') . "</p>\n";
echo "<p><strong>REQUEST_URI:</strong> " . htmlspecialchars($_SERVER['REQUEST_URI'] ?? 'Unknown') . "</p>\n";
echo "</body></html>\n";
?>