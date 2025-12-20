#include "tcpserver.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

TCPServer::TCPServer(const std::string &ipAddress, int port) : port(port), serverAddress(ipAddress)
{
	serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (serverSocket == -1) {
		exitWithError("Failed to create socket");
	}

	int opt = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	socketAddress = { 0, 0, 0, 0 };

	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(port);

	if (ipAddress.empty()) {
		socketAddress.sin_addr.s_addr = INADDR_ANY;
	} else {
		socketAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	}
}

TCPServer::TCPServer(TCPServer &&s)
{
	socketAddress = std::exchange(s.socketAddress, { 0, 0, 0, 0 });
	serverSocket = std::exchange(s.serverSocket, -1);
	port = std::exchange(s.port, 0);
	serverAddress = std::exchange(s.serverAddress, "");
}

TCPServer &TCPServer::operator=(TCPServer &&s)
{
	if (serverSocket != -1)
		close(serverSocket);
	socketAddress = std::exchange(s.socketAddress, { 0, 0, 0, 0 });
	serverSocket = std::exchange(s.serverSocket, -1);
	port = std::exchange(s.port, 0);
	serverAddress = std::exchange(s.serverAddress, "");
	return *this;
}

TCPServer::~TCPServer()
{
	if (serverSocket)
		close(serverSocket);
}

void TCPServer::exitWithError(const std::string &errorMessage)
{
	std::cerr << "Error: " << errorMessage << std::endl;
	exit(1);
}

void TCPServer::startServer()
{
	if (serverSocket == -1 || !port)
		exitWithError("Invalid server state");
	if (bind(serverSocket, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) == -1) {
		perror("bind");
		exitWithError("Failed to bind.");
	}
	if (listen(serverSocket, 5) < 0) {
		perror("listen");
		exitWithError("Failed to listen");
	}
}

int TCPServer::acceptConnection()
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	return accept(this->serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
}

int TCPServer::getSocket()
{
	return serverSocket;
}

void TCPServer::closeClient(int clientSocket)
{
	close(clientSocket);
}
