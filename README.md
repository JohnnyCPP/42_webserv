# Webserv

![image](./resource/42_madrid.jpg)

> 42 Madrid is an academy for values, attitude and learning "hard and soft skills" in the digital environment.

## Project Overview

Webserv is an implementation of an HTTP server written entirely in C++98. The project's goal is to understand the inner workings of the HTTP protocol, web servers, and low-level network programming.

Unlike using existing web servers like NGINX or Apache, this project requires building from the ground up: socket management, HTTP request parsing, response generation, CGI communication, and non-blocking I/O handling.

Core features:

- HTTP/1.1 compliant request parsing and response generation
- GET, POST, DELETE methods
- Non-blocking I/O multiplexing with poll()/epoll()/kqueue()
- Config file support (NGINX-style syntax)
- CGI execution (dynamic content generation)
- Static file serving with MIME type detection
- File upload handling
- Configurable error pages
- Multiple port listening

Developed by JohnnyCPP and igenez-y.

## Run

TODO

