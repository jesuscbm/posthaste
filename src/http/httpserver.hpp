#ifndef HTTP_SERVER
#define HTTP_SERVER
#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/epoll.h>
#include <vector>

#include <unordered_map>

#include "httprequest.hpp"
#include "httpresponse.hpp"
#include "tcpserver.hpp"
#include "threadpool.hpp"

class HttpServer {
   private:
	enum State { METHOD, PATH, VERSION, HEADERS_KEY, HEADERS_VALUE, BODY, DONE };

	struct ConnectionContext {	// Manages parsing in active connections
		int fd;

		State state = METHOD;
		HttpRequest req;
		std::string temp_method, temp_path, temp_version;
		std::string current_header_key, current_header_value;
		static constexpr int buf_max = 512;
		char buffer[buf_max];
		std::string body;
		size_t content_length = 0;

		ConnectionContext(int f) : fd(f)
		{
		}

		void reset()  // To be called after each request is parsed
		{
			state = METHOD;
			req = HttpRequest();
			temp_method.clear();
			temp_path.clear();
			temp_version.clear();
			current_header_key.clear();
			current_header_value.clear();
			body.clear();
			content_length = 0;
		}
	};

	std::optional<TCPServer> tcpServer;
	std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest &)>> endpoints;
	std::vector<std::pair<std::string, std::function<HttpResponse(const HttpRequest &)>>>
	  wildcard_endpoints;

	// Map with the context for each fd, that way a thread can resume the parsing of a request that
	// another thread started
	std::unordered_map<int, std::shared_ptr<ConnectionContext>> contexts;
	std::mutex contexts_mutex;	// unordered_map is not thread-safe

	ThreadPool tp;
	int epoll_fd;
	static constexpr int max_events = 10;
	struct epoll_event wait_events[max_events];

	void handle_connection(int fd);
	static void send_response(int fd, const std::string &response);
	static std::optional<HttpRequest> get_request(ConnectionContext &c, bool &is_closed);

   public:
	HttpServer(int port, std::optional<size_t> n_threads = std::nullopt);
	HttpServer(const HttpServer &) = delete;
	HttpServer(HttpServer &&) noexcept;
	auto &operator=(const HttpServer &) = delete;
	auto &operator=(HttpServer &&) noexcept;
	~HttpServer();

	void addEndpoint(const std::string &path, std::function<HttpResponse(const HttpRequest &)>);
	void serve(std::optional<std::reference_wrapper<std::atomic<bool>>> = std::nullopt);
};
#endif
