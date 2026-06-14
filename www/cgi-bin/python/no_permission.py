#!/usr/bin/env python3
import sys
import os

method = os.environ.get('REQUEST_METHOD', 'UNKNOWN')
content_length = os.environ.get('CONTENT_LENGTH', '0')
query_string = os.environ.get('QUERY_STRING', '')

# Read POST body from stdin
post_body = ""
if method == "POST":
    post_body = sys.stdin.read(int(content_length)) if content_length != '0' else ""

# Generate response
html = f"""<html>
<body>
<h1>Request Method: {method}</h1>
<h2>Environment Variables:</h2>
<ul>
<li>REQUEST_METHOD: {method}</li>
<li>CONTENT_LENGTH: {content_length}</li>
<li>QUERY_STRING: {query_string}</li>
</ul>
<h2>POST Body:</h2>
<pre>{post_body}</pre>
</body>
</html>"""

print("Content-Type: text/html")
print(f"Content-Length: {len(html)}")
print()
print(html)
