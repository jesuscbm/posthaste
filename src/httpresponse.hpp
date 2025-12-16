#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class HttpResponse {
   public:
	HttpResponse() = default;
	void setStatusCode(int code);
	void setContentType(const std::string &type);
	void setBody(const std::string &body);

	// Assembles the final string: Status + Headers + \r\n + Body
	std::string serialize() const;

   private:
	int code = 200;
	std::string body;
	std::map<std::string, std::string> headers;

	std::string getStatusText(int code) const;
};
