#!/usr/bin/env python3
import os

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Python CGI Test</h1>")
print("<p>QUERY_STRING: {}</p>".format(os.environ.get("QUERY_STRING", "")))
print("<p>REQUEST_METHOD: {}</p>".format(os.environ.get("REQUEST_METHOD", "")))
print("</body></html>")
