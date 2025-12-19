#include "httprequest.hpp"
#include <string>
#include <vector>

void HttpRequest::addHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

void HttpRequest::setMethod(std::string m)
{
	method = std::move(m);
}
void HttpRequest::setPath(std::string p)
{
	path = std::move(p);
}
void HttpRequest::setVersion(std::string v)
{
	version = std::move(v);
}
void HttpRequest::setBody(std::vector<char> b)
{
	body = std::move(b);
}

const std::string &HttpRequest::getMethod() const
{
	return method;
}

const std::string &HttpRequest::getPath() const
{
	return path;
}

const std::string &HttpRequest::getVersion() const
{
	return version;
}

const std::vector<char> &HttpRequest::getBody() const
{
	return body;
}

std::string HttpRequest::serialize() const
{
	std::string serialized;

	serialized.append(method).append(" ").append(path).append(" ").append(version).append("\r\n");

	for (auto it = headers.begin(); it != headers.end(); it++)
		serialized.append(it->first).append(":").append(it->second).append("\r\n");

	serialized.append(std::string(body.begin(), body.end()));

	return serialized;
}

std::optional<const std::string> HttpRequest::getHeader(const std::string &h) const
{
	auto it = headers.find(h);
	if (it == headers.end())
		return "";
	return it->second;
}
