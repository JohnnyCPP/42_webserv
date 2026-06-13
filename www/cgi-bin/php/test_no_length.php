#!/usr/bin/env php-cgi
<?php
// No Content-Length header - server must detect EOF
header("Content-Type: text/html");

echo "<html><body>";
echo "<h1>No Content-Length Header</h1>";
echo "<p>This response has no Content-Length. ";
echo "The server must detect EOF to know when the response ends.</p>";
echo str_repeat(".", 100);
echo "</body></html>";
?>
