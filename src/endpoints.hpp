#ifndef ENDPOINTS_HPP
#define ENDPOINTS_HPP

#include <string>

#include "http/httprequest.hpp"
#include "http/httpresponse.hpp"

HttpResponse serve_file(const HttpRequest &, std::string filename);
HttpResponse handle_paste(const HttpRequest &req);
HttpResponse show_paste(const HttpRequest &req);

#endif	// !ENDPOINTS_HPP
