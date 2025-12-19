#include <map>
#include <optional>
#include <string>
#include <vector>

class HttpRequest {
   private:
	std::string method;
	std::string path;
	std::string version;
	std::vector<char> body;
	std::map<std::string, std::string> headers;

   public:
	HttpRequest() = default;
	HttpRequest(const HttpRequest &) = default;
	HttpRequest(HttpRequest &&) = default;
	HttpRequest &operator=(const HttpRequest &) = default;
	HttpRequest &operator=(HttpRequest &&) = default;

	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::vector<char> &getBody() const;
	std::optional<const std::string> getHeader(const std::string &) const;

	void setMethod(std::string);
	void setPath(std::string);
	void setVersion(std::string);
	void setBody(std::vector<char>);
	void addHeader(const std::string &key, const std::string &value);

	std::string serialize() const;
};
