<?php
echo "Content-type: text/html\r\n\r\n";
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head><title>CGI Test</title></head>\n";
echo "<body>\n";
echo "<h1>CGI Test</h1>\n";
echo "<p>Hello from PHP!</p>\n";
echo "<h2>Environment Variables:</h2>\n";
echo "<ul>\n";
foreach ($_SERVER as $key => $value) {
    echo "<li><strong>$key:</strong> $value</li>\n";
}
echo "</ul>\n";
echo "<h2>GET Data:</h2>\n";
echo "<ul>\n";
foreach ($_GET as $key => $value) {
    echo "<li><strong>$key:</strong> $value</li>\n";
}
echo "</ul>\n";
echo "<h2>POST Data:</h2>\n";
echo "<ul>\n";
foreach ($_POST as $key => $value) {
    echo "<li><strong>$key:</strong> $value</li>\n";
}
echo "</ul>\n";
echo "</body>\n";
echo "</html>\n";
?>