#include <functional>
#include <optional>
#include <string>

#include <unordered_map>

#include "httprequest.hpp"
#include "httpresponse.hpp"
#include "tcpserver.hpp"

class HttpServer {
   private:
	std::optional<TCPServer> tcpServer;
	std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest &)>> endpoints;

   public:
	HttpServer(int port);
	HttpServer(const HttpServer &) = delete;
	HttpServer(HttpServer &&) noexcept;
	auto &operator=(const HttpServer &) = delete;
	auto &operator=(HttpServer &&) noexcept;
	~HttpServer() = default;

	void addEndpoint(const std::string &path, std::function<HttpResponse(const HttpRequest &)>);
	void serve();
};
