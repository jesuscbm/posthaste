#include "httpserver.hpp"
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <optional>
#include <sys/epoll.h>
#include <unistd.h>

std::optional<HttpRequest> HttpServer::get_request(ConnectionContext &ctx, bool &is_closed)
{
	const int fd = ctx.fd;
	HttpRequest req = ctx.req;
	std::string &body = ctx.body;

	for (;;) {
		ssize_t bytes_received = recv(fd, ctx.buffer, sizeof(ctx.buffer), 0);

		if (bytes_received <= 0) {
			// If we get 0 or the error doesn't say to try again we retry
			if (bytes_received == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK))
				is_closed = true;
			if (ctx.state >= BODY) {
				req.setBody(std::move(ctx.body));
				HttpRequest final_req = std::move(ctx.req);
				ctx.reset();
				return req;
			}
			return std::nullopt;
		}

		// TODO: Further checks (slowloris, long headers...)
		for (int i = 0; i < bytes_received; i++) {
			char c = ctx.buffer[i];
			switch (ctx.state) {
			case METHOD:
				if (c == ' ') {
					req.setMethod(std::move(ctx.temp_method));
					ctx.state = PATH;
				} else {
					ctx.temp_method += c;
				}
				break;
			case PATH:
				if (c == ' ') {	 // TODO: Clean path?
					req.setPath(std::move(ctx.temp_path));
					ctx.state = VERSION;
				} else {
					ctx.temp_path += c;
				}
				break;
			case VERSION:
				if (c == '\r')
					continue;
				if (c == '\n') {
					req.setVersion(std::move(ctx.temp_version));
					ctx.state = HEADERS_KEY;
				} else {
					ctx.temp_version += c;
				}
				break;

			case HEADERS_KEY:
				if (c == '\r')
					continue;
				if (c == '\n') {
					if (ctx.current_header_key.empty()) {
						// If we're done with headers (2 straight empty lines), we see if we need a
						// body
						if (ctx.content_length > 0) {
							ctx.state = BODY;
							body.reserve(ctx.content_length);
						} else {
							HttpRequest final_req = std::move(ctx.req);
							ctx.reset();
							return req;
						}
					}
					ctx.current_header_key.clear();
				} else if (c == ':') {
					ctx.state = HEADERS_VALUE;
				} else {
					ctx.current_header_key += c;
				}
				break;

			case HEADERS_VALUE:
				if (c == '\r')
					continue;
				if (c == '\n') {
					// If we're done with this value, we can add the header. And start again
					req.addHeader(ctx.current_header_key, ctx.current_header_value);

					if (ctx.current_header_key == "Content-Length") {
						ctx.content_length = std::stoi(ctx.current_header_value);
					}

					ctx.current_header_value.clear();
					ctx.current_header_key.clear();
					ctx.state = HEADERS_KEY;
				} else {
					// For the spaces after ':'. This was awful to debug
					if (ctx.current_header_value.empty() && c == ' ')
						continue;
					ctx.current_header_value += c;
				}
				break;

			case BODY:
				body.push_back(c);
				if (body.size() >= ctx.content_length) {
					ctx.state = DONE;
				}
				break;

			case DONE:
				// TODO: Save extra bytes for next request (Weird but possible)
				req.setBody(std::move(body));
				HttpRequest final_req = std::move(ctx.req);
				ctx.reset();
				return req;
			}
		}
	}

	// We haven't finished a request, so we return nullopt and wait for more
	return std::nullopt;
}

void HttpServer::send_response(int fd, const std::string &response)
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

void HttpServer::handle_connection(int fd)
{
	// First we get the context in a thread-safe way
	std::shared_ptr<ConnectionContext> ctx_ptr = nullptr;
	{
		std::lock_guard lock(contexts_mutex);
		if (contexts.find(fd) != contexts.end()) {
			ctx_ptr = contexts.at(fd);
		}
	}
	if (!ctx_ptr)
		return;
	ConnectionContext &c = *ctx_ptr;

	for (;;) {
		bool is_closed = false;
		std::optional<HttpRequest> request = get_request(c, is_closed);

		if (is_closed) {
			close(fd);
			{
				std::lock_guard lock(contexts_mutex);
				contexts.erase(fd);
			}
			return;
		}

		if (request) {
			const HttpResponse response = [&request, this] {
				const std::string &path = request->getPath();

				auto it = this->endpoints.find(path);
				if (it != this->endpoints.end()) {
					return it->second(*request);
				}

				for (const auto &[base_path, handler] : this->wildcard_endpoints) {
					if (path.rfind(base_path, 0) == 0) {
						return handler(*request);
					}
				}

				HttpResponse notFound;
				notFound.setStatusCode(404);
				notFound.setBody("<h1>404 Not found</h1>");
				return notFound;
			}();

			std::string s_response = response.serialize();
			send_response(c.fd, s_response);
		} else {
			break;
		}
	}

	// We used EPOLLONESHOT, so the socket is now ignored by epoll.
	// We must add it back so we get notified of the next packet.
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.fd = fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

HttpServer::~HttpServer()
{
	if (epoll_fd >= 0)
		close(epoll_fd);
}

HttpServer::HttpServer(int port, std::optional<size_t> n_threads) : tp(n_threads)
{
	epoll_fd = epoll_create1(0);
	tcpServer.emplace("", port);
	tcpServer->startServer();
}

auto &HttpServer::operator=(HttpServer &&s) noexcept
{
	tcpServer = std::move(s.tcpServer);
	endpoints = std::move(s.endpoints);
	contexts = std::move(s.contexts);
	epoll_fd = s.epoll_fd;
	s.epoll_fd = -1;

	tcpServer = std::move(s.tcpServer);
	s.tcpServer.reset();
	return *this;
}

HttpServer::HttpServer(HttpServer &&s) noexcept
	: tcpServer(std::move(s.tcpServer)),
	  endpoints(std::move(s.endpoints)),
	  contexts(std::move(s.contexts)),
	  epoll_fd(s.epoll_fd)
{
	s.epoll_fd = -1;
	s.tcpServer.reset();
}

void HttpServer::addEndpoint(const std::string &path,
							 std::function<HttpResponse(const HttpRequest &)> f)
{
	if (!path.empty() && path.back() == '*') {
		std::string base_path = path.substr(0, path.size() - 1);
		wildcard_endpoints.push_back({ base_path, f });
	} else {
		endpoints[path] = f;
	}
}

void HttpServer::serve(std::optional<std::reference_wrapper<std::atomic<bool>>> stop)
{
	int socketfd = tcpServer->getSocket();
	// We add the listen socket monitor, which will accept connections.
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = socketfd;

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socketfd, &ev);

	while (stop == std::nullopt || !stop->get().load()) {
		int n_fds = epoll_wait(epoll_fd, wait_events, max_events, -1);

		if (n_fds < 0) {
			if (errno == EINTR && stop->get().load())
				break;
			continue;
		}

		for (int i = 0; i < n_fds; i++) {
			int fd = wait_events[i].data.fd;
			if (fd == socketfd) {
				for (;;) {	// Loop accept due to Edge Triggered mode
					// Activity on socket => We can accept a new connection
					int new_fd = tcpServer->acceptConnection();

					if (new_fd < 0)
						break;

					// Non-blocking
					int flags = fcntl(new_fd, F_GETFL, 0);
					fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);

					struct epoll_event new_ev;
					new_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
					new_ev.data.fd = new_fd;

					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &new_ev);

					{
						std::lock_guard lock(contexts_mutex);
						contexts[new_fd] = std::make_shared<ConnectionContext>(new_fd);
					}
				}
				continue;
			}

			tp.addTask([this, fd] { this->handle_connection(fd); });
		}
	}
}
