#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");

echo "<html>";
echo "<head><title>PHP CGI Test</title></head>";
echo "<body>";
echo "<h1>PHP CGI Script Working!</h1>";
echo "<h2>Environment Variables</h2>";
echo "<table border='1'>";

$env_vars = array(
    "REQUEST_METHOD",
    "QUERY_STRING",
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "PATH_INFO",
    "SCRIPT_NAME",
    "SCRIPT_FILENAME",
    "SERVER_PROTOCOL",
    "SERVER_NAME",
    "HTTP_USER_AGENT"
);

foreach ($env_vars as $var) {
    $value = getenv($var);
    if ($value === false) {
        $value = "NOT SET";
    }
    echo "<tr><td>$var</td><td>$value</td></tr>";
}

echo "<table>";

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    $input = file_get_contents('php://input');
    if (!empty($input)) {
        echo "<h2>POST Body</h2>";
        echo "<pre>" . htmlspecialchars($input) . "</pre>";
    }
}

echo "</body>";
echo "</html>";
?>
