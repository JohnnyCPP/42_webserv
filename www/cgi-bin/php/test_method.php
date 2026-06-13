#!/usr/bin/env php-cgi
<?php
$method = $_SERVER['REQUEST_METHOD'] ?? 'UNKNOWN';
$content_length = $_SERVER['CONTENT_LENGTH'] ?? '0';
$query_string = $_SERVER['QUERY_STRING'] ?? '';
$content_type = $_SERVER['CONTENT_TYPE'] ?? '';

// Read POST body from stdin
$post_body = '';
if ($method === 'POST') {
    $post_body = file_get_contents('php://stdin');
}

$html = <<<HTML
<html>
<body>
<h1>Request Method: $method</h1>
<h2>Environment Variables:</h2>
<ul>
<li>REQUEST_METHOD: $method</li>
<li>CONTENT_LENGTH: $content_length</li>
<li>CONTENT_TYPE: $content_type</li>
<li>QUERY_STRING: $query_string</li>
</ul>
<h2>POST Body:</h2>
<pre>$post_body</pre>
</body>
</html>
HTML;

header("Content-Type: text/html");
header("Content-Length: " . strlen($html));
echo $html;
?>
