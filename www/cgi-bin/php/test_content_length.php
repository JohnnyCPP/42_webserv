#!/usr/bin/env php-cgi
<?php
// Return a custom Content-Length header
$body = str_repeat("X", 100);
$content_length = 50; // Lie about length

header("Content-Type: text/plain");
header("Content-Length: $content_length");
echo $body; // Actually sends 100 bytes
?>
