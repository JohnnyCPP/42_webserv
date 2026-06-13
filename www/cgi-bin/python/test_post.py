#!/usr/bin/env python3
import sys

# Read POST body
body = sys.stdin.read()
response_body = "<html><body><h1>POST Received</h1><p>You sent: %s</p></body></html>" % body

print("Content-Type: text/html")
print("Content-Length: %d" % len(response_body))
print()
print(response_body)
