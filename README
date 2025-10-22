# Webserv Project

## Overview
Webserv is a lightweight HTTP server implementation designed to handle CGI scripts and static files. It is built in C++ and adheres to the HTTP/1.1 protocol. The project includes custom implementations for handling requests, responses, and server configurations.

## Features
- CGI script execution for dynamic content.
- Static file serving with MIME type handling.
- Customizable server configurations via `webserv.conf`.
- Error handling with custom error pages.
- Logging for debugging and monitoring.

## Project Structure
```
Makefile          # Build system
README            # Project documentation
stress_test.sh    # Script for stress testing the server
webserv           # Main server executable
webserv.conf      # Server configuration file
webserv.yml       # Alternative configuration file
cgi-bin/          # Directory for CGI scripts
errors/           # Custom error pages
obj/              # Compiled object files
src/              # Source code directory
static/           # Static files (HTML, CSS, etc.)
uploads/          # Directory for uploaded files
```

### Key Directories
- **`src/`**: Contains the source code for the server, organized into modules like `cgi`, `config`, `core`, `http`, and `utils`.
- **`cgi-bin/`**: Includes sample CGI scripts in Python and PHP.
- **`static/`**: Contains static assets such as HTML, CSS, and images.
- **`errors/`**: Custom error pages for HTTP status codes.

## Build Instructions
1. Ensure you have `make` and a C++ compiler installed.
2. Run the following command to build the project:
   ```
   make
   ```
3. The `webserv` executable will be generated in the root directory.

## Usage
1. Start the server by running:
   ```
   ./webserv webserv.conf
   ```
2. Access the server in your browser at `http://localhost:8080` (default port).
3. Modify `webserv.conf` to customize the server settings.

## Testing

### Stress Testing
Use the `stress_test.sh` script or the `siege` tool to perform stress testing on the server:

- Simulate 10 concurrent users for 1 minute:
  ```
  siege -c 10 -t 1M http://localhost:8080/index.html
  ```
- Benchmark the server in non-interactive mode:
  ```
  siege -b http://localhost:8080/index.html
  ```

### Memory Leak Testing
Use `valgrind` to check for memory leaks and track origins of uninitialized values:

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes ./webserv webserv.conf
```

## Dependencies
- Standard C++ libraries.
- No external dependencies are required.
