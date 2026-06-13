#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html")
print()

print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>CGI Script Working!</h1>")
print("<h2>Environment Variables</h2>")
print("<table border='1'>")

env_vars = [
    "REQUEST_METHOD",
    "QUERY_STRING",
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "PATH_INFO",
    "SCRIPT_NAME",
    "SCRIPT_FILENAME",
    "SERVER_PROTOCOL",
    "SERVER_NAME"
]

for var in env_vars:
    value = os.environ.get(var, "NOT SET")
    print(f"<tr><td>{var}</td><td>{value}</td></tr>")

print("</table>")

if os.environ.get("REQUEST_METHOD") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_length > 0:
        body = sys.stdin.read(content_length)
        print("<h2>POST Body</h2>")
        print(f"<pre>{body}</pre>")

print("</body>")
print("</html>")
