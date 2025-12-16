#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "httpserver.hpp"

using namespace std;

HttpResponse status(const HttpRequest &request)
{
	HttpResponse response;
	response.setStatusCode(200);
	response.setBody("{status:ok}");

	cout << request.serialize();

	return response;
}

int main()
{
	HttpServer server(8080);

	server.addEndpoint("/health", status);

	server.serve();

	return 0;
}
