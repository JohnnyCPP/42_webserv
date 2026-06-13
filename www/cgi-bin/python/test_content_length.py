#!/usr/bin/env python3
import sys

# CGI output with explicit Content-Length header
body = "<html><body><h1>CGI with Content-Length</h1><p>This response includes a Content-Length header.</p></body></html>"
content_length = len(body)

print("Content-Type: text/html")
print("Content-Length: %d" % content_length)
print()
print(body)
