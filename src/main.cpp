#include <csignal>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "httpserver.hpp"
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

	cout << "-- MAIN --\n" << request.serialize() << "\n --------- \n";

	return response;
}

int main()
{
	HttpServer server(8080, 1);

	signal(SIGINT, signal_handler);

	server.addEndpoint("/health", status);
	server.addEndpoint("/", status);

	server.serve(stop_signal);

	cout << "Exiting!\n";

	return 0;
}
