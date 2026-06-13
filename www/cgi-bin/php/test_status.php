#!/usr/bin/env php-cgi
<?php
// Return custom HTTP status
$message = "I'm a teapot";

header("Status: 418 $message");
header("Content-Type: text/html");

echo "<html><body><h1>418 $message</h1>";
echo "<p>This CGI returned a custom status code.</p>";
echo "</body></html>";
?>
