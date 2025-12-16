#include <map>
#include <string>
#include <vector>

struct HttpRequest {
	std::string method;
	std::string path;
	std::string version;
	std::vector<char> body;
	std::map<std::string, std::string> headers;

	HttpRequest() = default;
	HttpRequest(const HttpRequest &) = default;
	HttpRequest(HttpRequest &&) = default;
	HttpRequest &operator=(const HttpRequest &) = default;
	HttpRequest &operator=(HttpRequest &&) = default;

	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::vector<char> &getBody() const;
	const std::string &getHeader(const std::string &) const;

	void setMethod(const std::string &);
	void setPath(const std::string &);
	void setBody(const std::vector<char> &);
	void addHeader(const std::string &key, const std::string &value);

	std::string serialize() const;
};
