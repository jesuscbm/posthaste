#include "httpresponse.hpp"

void HttpResponse::setStatusCode(int code)
{
	this->code = code;
}

void HttpResponse::setContentType(std::string type)
{
	headers["Content-Type"] = std::move(type);
}

void HttpResponse::setBody(std::string body)
{
	this->body = body;
	headers["Content-Length"] = std::to_string(body.size());
}

void HttpResponse::addHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

std::string HttpResponse::serialize() const
{
	std::string ss;

	ss.append(getStatusText(this->code)).append("\r\n");

	for (const auto &p : this->headers)
		ss.append(p.first).append(": ").append(p.second).append("\r\n");

	ss.append("\r\n").append(this->body).append("\r\r\n\n");

	return ss;
}

std::string HttpResponse::getStatusText(int code) const
{
	std::string text;
	switch (code) {
	case 200:
		text = "OK";
		break;
	case 404:
		text = "Not Found";
		break;
	case 500:
		text = "Internal Server Error";
		break;
	default:
		text = "Unknown";
		break;
	}
	return "HTTP/1.1 " + std::to_string(code) + " " + text;
}
