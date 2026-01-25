# POSTHaste / Custom C++ HTTP Server

POSTHaste is a lightweight Pastebin service.

The real deal is a high-performance, non-blocking HTTP server API built from scratch in C++17. It implements the Reactor pattern using `epoll` (Edge Triggered) and a custom ThreadPool.

## Architecture & Features

- **Core:** Non-blocking I/O with `epoll` in Edge-Triggered mode.
- **Concurrency:** Custom `ThreadPool` for task distribution (Reactor pattern).
- **Parsing:** Hand-written HTTP 1.1 state machine (Zero-copy intent).
- **Application (Pastebin):**
  - **Storage:** Flat-file system storage in the `p/` directory.
  - **IDs:** Random Base62 ID generation.
  - **Expiration:** Lazy expiration strategy (checks metadata on read).
  - **Routing:** Wildcard support (e.g., `/p/*`).

## Project Structure Overview

- **src/**: Contains the main application logic, endpoints, and utilities.
- **src/http/**: Houses the core server infrastructure, including the HTTP state machine parser, response serializer, and low-level TCP socket wrappers.
- **p/**: The data storage directory where pastes and their metadata are saved.
- **index.html**: The frontend interface for the Pastebin.

## Compilation

Standard GNU Make build system.

```
make release
```

## Usage

1.  **Run the server:**

    ```bash
    ./server [-p <PORT>] [-w <N_WORKERS>]
    ```

    Listens on port `80` by default.

    A docker image is available in the ghcr:

    ```bash
    docker run -p"80:80" -v"$PWD/p:/p" -d ghcr.io/jesuscbm/posthaste:latest
    ```

2.  **Web Interface:**
    Access `http://localhost:80` to use the frontend.

3.  **API / Endpoints:**

    | Method | Path      | Description                                                           |
    | :----- | :-------- | :-------------------------------------------------------------------- |
    | `GET`  | `/`       | Serves `index.html`.                                                  |
    | `GET`  | `/health` | Server status check.                                                  |
    | `POST` | `/paste`  | Accepts `content` and `expiration` (form-data). Returns 303 Redirect. |
    | `GET`  | `/p/*`    | Retrieves paste by ID. Handles lazy deletion if expired.              |

## Storage Logic

Pastes are stored as file pairs in sharded subdirectories inside the `p/` directory:

- `{ID}`: Raw content.
- `{ID}.meta`: Expiration timestamp (Unix epoch).

When a paste is requested via `GET /p/{ID}`, the server reads the `.meta` file. If `current_time > expiration`, both files are physically deleted and a 404 is returned.

## Docker Image

A docker image can be built with:

```
docker -t posthaste build
```

To run in port 8080 and save the pastes in the folder `./p`:

```
docker run -p"8080:80" -v"$PWD/p:/p" posthaste:latest
```

## License

**GNU General Public License v3.0**
