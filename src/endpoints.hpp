#ifndef ENDPOINTS_HPP
#define ENDPOINTS_HPP

#include "http/httprequest.hpp"
#include "http/httpresponse.hpp"

HttpResponse root_endpoint(const HttpRequest &req);
HttpResponse handle_paste(const HttpRequest &req);
HttpResponse show_paste(const HttpRequest &req);

#endif	// !ENDPOINTS_HPP
