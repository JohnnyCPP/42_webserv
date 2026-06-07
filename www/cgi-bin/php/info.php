#!/usr/bin/env php-cgi
<?php
echo "Content-Type: text/html\n\n";
echo "<html><body>";
echo "<h1>PHP CGI Test</h1>";
echo "<p>REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<p>QUERY_STRING: " . $_SERVER['QUERY_STRING'] . "</p>";
echo "</body></html>";
?>
