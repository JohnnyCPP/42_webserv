#!/usr/bin/env python3

# CGI output with custom status (404)
body = "<html><body><h1>404 Custom Error</h1><p>This CGI returns a 404 status.</p></body></html>"

print("Status: 404 Not Found")
print("Content-Type: text/html")
print()
print(body)
