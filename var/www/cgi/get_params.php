<?php
echo "Content-type: text/html\r\n\r\n";
echo "<!DOCTYPE html>\n";
echo "<html><head><title>GET Parameters</title></head>\n";
echo "<body>\n";
echo "<h1>GET Parameters Test</h1>\n";
echo "<h2>Parameters Received:</h2>\n";
echo "<ul>\n";
foreach ($_GET as $key => $value) {
    echo "<li><strong>" . htmlspecialchars($key) . ":</strong> " . htmlspecialchars($value) . "</li>\n";
}
echo "</ul>\n";
echo "</body></html>\n";
?>