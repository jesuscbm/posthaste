#include "endpoints.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

#include "utils.hpp"

HttpResponse serve_file(const HttpRequest &, std::string filename)
{
	std::ifstream f(filename);

	if (!f.is_open()) {
		HttpResponse response;
		response.setStatusCode(404);
		response.setBody("<h1>404 Not Found. Failed to serve file</h1>");
		return response;
	}

	HttpResponse response;

	std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	response.setStatusCode(200);
	response.setContentType("text/html");
	response.setBody(std::move(s));

	return response;
}

HttpResponse handle_paste(const HttpRequest &req)
{
	HttpResponse response;
	if (req.getMethod() != "POST") {
		response.setStatusCode(403);
		response.setBody("<h1>403 Method Not Allowed</h1>");
		return response;
	}

	auto v = req.getBody();
	auto form_data = parse_form_data(std::string(v.begin(), v.end()));

	auto it_content = form_data.find("content");
	auto it_expiration = form_data.find("expiration");
	if (it_content == form_data.end() || it_expiration == form_data.end()) {
		response.setStatusCode(400);
		response.setBody("<h1>Incorrect Body</h1>");
		return response;
	}

	std::string id = generate_id();
	save_paste_to_disk(id, it_content->second, it_expiration->second);

	std::string url = "/p/" + id;

	response.setStatusCode(303);
	response.addHeader("Location", url);

	return response;
}

HttpResponse show_paste(const HttpRequest &req)
{
	std::string path = req.getPath();

	HttpResponse response;
	if (path.size() <= 6) {
		response.setStatusCode(404);
		response.setBody("<h1>Not found</h1>");
		return response;
	}

	std::string paste_id = path.substr(3);
	std::string dirpath = "p/" + paste_id.substr(0, 1) + "/" + paste_id.substr(1, 1) + "/";
	std::string filepath = dirpath + paste_id.substr(2);
	std::string metapath = dirpath + paste_id.substr(2) + ".meta";

	std::fstream f(filepath);
	if (!f.is_open()) {
		response.setStatusCode(404);
		response.setBody("<h1>Not found</h1>");
		return response;
	}

	std::ifstream meta(metapath);
	if (!meta.is_open()) {
		response.setStatusCode(404);
		response.setBody("<h1>Not found</h1>");
		return response;
	}

	long long expiration;
	meta >> expiration;

	if (expiration != -1 && std::time(nullptr) > expiration) {
		f.close();
		meta.close();
		std::filesystem::remove(filepath);
		std::filesystem::remove(metapath);

		if (std::filesystem::is_empty(dirpath))
			std::filesystem::remove(dirpath);

		response.setStatusCode(404);
		response.setBody("<h1>Not found</h1>");
		return response;
	}

	std::stringstream ss;
	ss << f.rdbuf();

	std::string format_date;
	if (expiration == -1) {
		format_date = "Never";
	} else {
		std::time_t t = static_cast<std::time_t>(expiration);
		std::tm tm;
		localtime_r(&t, &tm);
		std::stringstream cpp_yousuck_sometimes;
		cpp_yousuck_sometimes << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		format_date = cpp_yousuck_sometimes.str();
	}

	std::string page = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Paste )" + paste_id
					   + R"(</title>
        <style>
            body { background: #1a1b26; color: #a9b1d6; font-family: monospace; padding: 20px; }
            .code-block { 
                background: #16161e; 
                padding: 20px; 
                border-radius: 8px; 
                border: 1px solid #292e42; 
                white-space: pre-wrap; 
                overflow-x: auto;
            }
            /* Contenedor flexible para alinear link y fecha */
            .header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 20px;
            }
            a { color: #7aa2f7; text-decoration: none; }
            .meta { color: #565f89; font-size: 0.85rem; } 
        </style>
    </head>
    <body>
        <div class="header">
            <a href="/">&larr; Create New Paste</a>
            <span class="meta">Expires: )"
					   + format_date + R"(</span>
        </div>
        <div class="code-block">)"
					   + html_escape(ss.str()) + R"(</div>
    </body>
    </html>
    )";

	response.setBody(std::move(page));
	response.setContentType("text/html");

	return response;
}
