#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class TCPServer {
   private:
	struct sockaddr_in socketAddress;
	int port, serverSocket;
	std::string serverAddress;

	void exitWithError(const std::string &errorMessage);

   public:
	TCPServer(const std::string &ipAddress, int port);
	TCPServer(const TCPServer &) = delete;
	TCPServer(TCPServer &&s);
	auto &operator=(const TCPServer &) = delete;
	TCPServer &operator=(TCPServer &&);
	~TCPServer();

	void startServer();
	int acceptConnection();
	int getSocket();
	void closeClient(int clientSocket);
};
