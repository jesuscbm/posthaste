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

HttpResponse status(const HttpRequest &)
{
	HttpResponse response;
	response.setStatusCode(200);
	response.setBody("{\"status\":\"ok\"}");
	response.setContentType("application/json");

	return response;
}

int main(int argc, char *argv[])
{
	int port = 80, n_threads = thread::hardware_concurrency();

	if (argc == 1) {
		cout << "Using default values:\nPort 80, Number of workers: " << n_threads << endl;
	}

	for (int i = 1; i < argc - 1; i++) {
		string_view arg(argv[i]);
		if (arg == "-p") {
			port = stoi(argv[i + 1]);
			i++;
		} else if (arg == "-w") {
			n_threads = stoi(argv[i + 1]);
			i++;
		}
	}

	HttpServer server(port, n_threads);

	signal(SIGINT, signal_handler);

	server.addEndpoint("/health", status);
	server.addEndpoint("/", root_endpoint);
	server.addEndpoint("/paste", handle_paste);
	server.addEndpoint("/p/*", show_paste);

	server.serve(stop_signal);

	cout << "\nExiting!\n";

	return 0;
}
