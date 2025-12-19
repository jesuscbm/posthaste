# C++ HTTP Server

A lightweight, custom HTTP server implementation written in C++ using POSIX
sockets, using epoll and a threadpool (reactor pattern). It features a custom
HTTP 1.1 request parser and functional endpoint routing.

## Project Structure

- **main.cpp**: Entry point that initializes the server and defines endpoints.
- **httpserver.cpp/hpp**: Manages the connection loop and routes requests to
  functions.
- **tcpserver.cpp/hpp**: Wraps low-level POSIX socket operations (socket, bind,
  listen, accept).
- **httprequest.cpp/hpp**: Parses and stores HTTP method, path, headers, and
  body.
- **httpresponse.cpp/hpp**: Helper class to format and serialize HTTP
  responses.
- **threadpool.cpp/hpp**: Class that manages the concurrency as a threadpool
  with a task queue.

## Requirements

- C++17 compatible compiler.
- Linux/Unix environment.

## Compilation

Compile the source files using `g++`.

```bash
make
```

## Usage

1.  **Start the server:**

    ```bash
    ./server
    ```

    The default configuration listens on port 8080.

2.  **Define Endpoints:**

    Routes are defined using the `addEndpoint` method, accepting a path string
    and a callback function.

    ```cpp
    #include "httpserver.hpp"

    int main() {
    HttpServer server(8080);

        // Define a health check endpoint
        server.addEndpoint("/health", [](const HttpRequest &req) {
            HttpResponse response;
            response.setStatusCode(200);
            response.setBody("OK");
            return response;
        });

        server.serve();
        return 0;

    }
    ```

## TODO

- Use HTTPServer API to create a Pastebin-like web.
- Clean stop to server.
- New API for high I/O.
