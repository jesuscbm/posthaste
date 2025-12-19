#include <csignal>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "endpoints.hpp"
#include "http/httpserver.hpp"

using namespace std;

std::atomic<bool> stop_signal(false);

void signal_handler(int)
{
	stop_signal = true;
}

HttpResponse status(const HttpRequest &request)
{
	HttpResponse response;
	response.setStatusCode(200);
	response.setBody(*request.getHeader("Host"));

	return response;
}

int main()
{
	HttpServer server(8080);

	signal(SIGINT, signal_handler);

	server.addEndpoint("/health", status);
	server.addEndpoint("/", [](const HttpRequest &r) { return serve_file(r, "index.html"); });
	server.addEndpoint("/paste", handle_paste);
	server.addEndpoint("/p/*", show_paste);

	server.serve(stop_signal);

	cout << "\nExiting!\n";

	return 0;
}
