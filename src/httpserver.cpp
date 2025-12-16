#include "httpserver.hpp"
#include <iostream>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>

// Private functions
#define HTTP_BUF_MAX 1024

enum State { METHOD, PATH, VERSION, HEADERS_KEY, HEADERS_VALUE, BODY, DONE };

std::optional<HttpRequest> get_request(int fd)
{
	State state = METHOD;
	char buf[HTTP_BUF_MAX];
	HttpRequest req;
	std::string current_header_key, current_header_value;
	size_t content_length = 0;

	for (;;) {
		ssize_t bytes_received = recv(fd, buf, sizeof(buf), 0);

		if (bytes_received < 0)
			return std::nullopt;
		if (bytes_received == 0)
			return std::nullopt;

		// TODO: Further checks (slowloris, long headers...)
		for (char c : buf) {
			switch (state) {
			case METHOD:
				if (c == ' ') {
					state = PATH;
				} else {
					req.method += c;
				}
				break;
			case PATH:
				if (c == ' ') {	 // TODO: Clean path?
					state = VERSION;
				} else {
					req.path += c;
				}
				break;
			case VERSION:
				if (c == '\r')
					continue;
				if (c == '\n') {
					state = HEADERS_KEY;
				} else {
					req.version += c;
				}
				break;

			case HEADERS_KEY:
				if (c == '\r')
					continue;
				if (c == '\n') {
					if (current_header_key.empty()) {
						// If we're done with headers (2 straight empty lines), we see if we need a
						// body
						if (content_length > 0) {
							state = BODY;
							req.body.reserve(content_length);
						} else {
							state = DONE;
						}
					}
					current_header_key.clear();
				} else if (c == ':') {
					state = HEADERS_VALUE;
				} else {
					current_header_key += c;
				}
				break;

			case HEADERS_VALUE:
				if (c == '\r')
					continue;
				if (c == '\n') {
					// If we're done with this value, we can add the header. And start again
					req.headers[current_header_key] = current_header_value;

					if (current_header_key == "Content-Length") {
						content_length = std::stoi(current_header_value);
					}

					current_header_value.clear();
					current_header_key.clear();
					state = HEADERS_KEY;
				} else {
					// For the spaces after ':'. This was awful to debug
					if (current_header_value.empty() && c == ' ')
						continue;
					current_header_value += c;
				}
				break;

			case BODY:
				req.body.push_back(c);
				if (req.body.size() >= content_length) {
					state = DONE;
				}
				break;

			case DONE:
				// TODO: Save extra bytes for next request
				return req;
			}
		}
	}

	return req;
}

void send_response(int fd, const std::string &response)
{
	const char *str = response.c_str();
	int total_sent = 0, to_send = response.size();

	for (;;) {
		ssize_t bytes_sent = send(fd, str + total_sent, to_send, MSG_NOSIGNAL);

		if (bytes_sent < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			return;
		}
		if (bytes_sent == 0)
			return;

		total_sent += bytes_sent;
		to_send -= bytes_sent;
	}
}

HttpServer::HttpServer(int port)
{
	tcpServer.emplace("", port);
	tcpServer->startServer();
}

auto &HttpServer::operator=(HttpServer &&s) noexcept
{
	tcpServer = std::move(s.tcpServer);
	s.tcpServer.reset();
	return *this;
}

HttpServer::HttpServer(HttpServer &&s) noexcept : tcpServer(std::move(s.tcpServer))
{
	s.tcpServer.reset();
}

void HttpServer::addEndpoint(const std::string &path,
							 std::function<HttpResponse(const HttpRequest &)> f)
{
	endpoints[path] = f;
}

void HttpServer::serve()
{
	while (true) {	// TODO: Cómo cerrarlo + Multihilo
		int fd = tcpServer->acceptConnection();

		std::optional<HttpRequest> request = get_request(fd);
		if (!request)
			continue;

		const HttpResponse response = [&request, this] {
			auto it = this->endpoints.find(request->path);
			if (it == this->endpoints.end()) {
				HttpResponse notFoud;
				notFoud.setStatusCode(404);
				notFoud.setBody("<h1>404 Not found</h1>");
				return notFoud;
			} else {
				return it->second(*request);
			}
		}();

		std::string s_response = response.serialize();
		send_response(fd, s_response);

		std::cerr << "Sent something: " << s_response << "\n";
	}
}
