*This project has been created as part of the 42 curriculum by jonnavar, igenez-y.*

## Description

Webserv is an implementation of an HTTP server written entirely in C++98. The project's goal is to understand the inner workings of the HTTP protocol, web servers, and low-level network programming.

Unlike using existing web servers like NGINX or Apache, this project requires building from the ground up: Socket management, HTTP request parsing, response generation, CGI communication, and non-blocking I/O handling.

Core features:

- HTTP/1.1 compliant request parsing and response generation
- GET, POST, DELETE methods
- Non-blocking I/O multiplexing with poll()
- Config file support (NGINX-style syntax)
- CGI execution
- Static file serving with MIME type detection
- File upload handling
- Configurable error pages
- Multiple port listening

## Instructions

Compile the project:

```bash
make
```

Additional Makefile rules:

- make clean - Remove object files

- make fclean - Remove object files and executable

- make re - Recompile from scratch

- make help - Show help

- make sanitize - Compile with fsanitize

Run the program with a configuration file:

```bash
./webserv [path/to/configuration_file]
```

## Resources

Here's a list of references consulted for this project:

- Reddit posts

- Wikipedia articles

- RFC 7230, RFC 7231, RFC 7232, RFC 7233, RFC 7234, RFC 7235

- The manual of Linux

- Our notes in Notion

AI was not used for any task or any part of the project.
