#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class HttpResponse {
   public:
	HttpResponse() = default;
	void setStatusCode(int code);
	void setContentType(std::string type);
	void setBody(std::string body);
	void addHeader(const std::string &, const std::string &);

	std::string serialize() const;

   private:
	int code = 200;
	std::string body;
	std::map<std::string, std::string> headers;

	std::string getStatusText(int code) const;
};

#endif	// !HTTP_RESPONSE
