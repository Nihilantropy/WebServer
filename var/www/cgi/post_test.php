<?php
echo "Content-type: text/html\r\n\r\n";
echo "<!DOCTYPE html>\n";
echo "<html><head><title>POST Test</title></head>\n";
echo "<body>\n";
echo "<h1>POST Data Test</h1>\n";
echo "<h2>POST Data Received:</h2>\n";
echo "<ul>\n";
foreach ($_POST as $key => $value) {
    echo "<li><strong>" . htmlspecialchars($key) . ":</strong> " . htmlspecialchars($value) . "</li>\n";
}
echo "</ul>\n";
echo "</body></html>\n";
?>