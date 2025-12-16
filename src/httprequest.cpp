#include "httprequest.hpp"
#include <string>
#include <vector>

void HttpRequest::addHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

void HttpRequest::setMethod(const std::string &m)
{
	method = m;
}
void HttpRequest::setPath(const std::string &p)
{
	path = p;
}
void HttpRequest::setBody(const std::vector<char> &b)
{
	body = b;
}

const std::string &HttpRequest::getMethod() const
{
	return method;
}

const std::string &HttpRequest::getPath() const
{
	return path;
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

	return serialized;
}

const std::string &HttpRequest::getHeader(const std::string &h) const
{
	return headers.at(h);
}
