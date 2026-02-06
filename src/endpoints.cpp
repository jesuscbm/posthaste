#include "endpoints.hpp"
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <string>

#include "http/httpresponse.hpp"
#include "utils.hpp"

// Curlable menu
#define R  "\033[0m"
#define B  "\033[1m"
#define P  "\033[38;5;141m"
#define BL "\033[38;5;111m"
#define G  "\033[38;5;120m"
#define GR "\033[38;5;240m"
#define RED "\033[38;5;203m"
#define HOST "paste.jesusblazquez.eu"

static constexpr std::string_view HELP_MENU = 
    "\n" P "   ~/posthaste " R ":: " BL "https://" HOST R "\n\n"

    B "   USAGE" R "\n"
      "     Send a POST request with " B "x-www-form-urlencoded" R " body.\n"
      "     Params " BL "content" R " & " BL "expiration" R " are required.\n\n"

    B "   UPLOAD METHODS" R "\n"
    GR "     # 1. Upload short text" R "\n"
       "     " G "$ curl -d \"content=Hello World\" -d \"expiration=1h\" " HOST "/paste" R "\n\n"

    GR "     # 2. Upload a file (Use --data-urlencode to safe escape)" R "\n"
       "     " G "$ curl --data-urlencode \"content@main.cpp\" -d \"expiration=1h\" " HOST "/paste" R "\n\n"

    GR "     # 3. Pipe from stdin (Shell substitution required)" R "\n"
       "     " G "$ curl --data-urlencode \"content=$(cat)\" -d \"expiration=1h\" " HOST "/paste" R "\n\n"

    B "   OPTIONS" R "\n"
       "     " BL "-d \"expiration=...\"" R "    -1, 1h, 1d, 1w\n\n"

    B "   EXAMPLES" R "\n"
    GR "     # Paste file with 1 hour expiration" R "\n"
       "     " G "$ curl --data-urlencode \"content@log.txt\" -d \"expiration=1h\" " HOST "/paste" R "\n\n";


HttpResponse serve_file(const HttpRequest &, std::string filename)
{
	std::ifstream f(filename, std::ios::ate);

	if (!f.is_open()) {
		HttpResponse response;
		response.setStatusCode(404);
		response.setBody("<h1>404 Not Found. Failed to serve file</h1>");
		return response;
	}

	HttpResponse response;

	std::size_t size = f.tellg();
	f.seekg(0);
	std::string s(size, '\0');
	f.read(&s[0], size);

	response.setStatusCode(200);
	response.setContentType("text/html");
	response.setBody(std::move(s));

	return response;
}

HttpResponse root_endpoint(const HttpRequest &req) {
	std::optional<std::string> user_agent = req.getHeader("User-Agent");
	if (!user_agent || user_agent->rfind("curl", 0) != 0){
		return serve_file(req, "index.html");
	}

	HttpResponse response;

	response.setBody(std::string(HELP_MENU));

	response.setContentType("text/plain");
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
	try {
		save_paste_to_disk(id, it_content->second, it_expiration->second);
	} catch (std::exception &ex) {
		response.setStatusCode(500);
		response.setBody("<h1>Internal Server Error</h1>");
		return response;
	}

	std::string url = "/p/" + id;

	std::optional<std::string> user_agent = req.getHeader("User-Agent");
	if (user_agent && user_agent->rfind("curl", 0) == 0){
		response.setBody(url + "\n");
		return response;
	}

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

	// We don't want /p/../../../danger
	if (paste_id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
		!= std::string::npos) {
		HttpResponse response;
		response.setStatusCode(400);
		response.setBody("<h1>Invalid Paste ID</h1>");
		return response;
	}

	std::string dirpath = "p/" + paste_id.substr(0, 1) + "/" + paste_id.substr(1, 1) + "/";
	std::string filepath = dirpath + paste_id.substr(2);
	std::string metapath = dirpath + paste_id.substr(2) + ".meta";

	std::ifstream f(filepath, std::ios::ate);
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

	std::size_t size = f.tellg();
	f.seekg(0);
	std::string paste(size, '\0');
	f.read(&paste[0], size);

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

	std::string page;
	page.reserve(4096 + size);

	std::optional<std::string> user_agent = req.getHeader("User-Agent");
	if (user_agent && user_agent->rfind("curl", 0) == 0){
		response.setBody(std::move(paste));
		response.setContentType("text/plain");
		return response;
	}

	page += R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Paste )" ;
	page += paste_id;
	page += R"(</title>
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
            <span class="meta">Expires: )";
	page += format_date;
	page += R"(</span>
        </div>
        <div class="code-block">)"
					   + html_escape(paste) + R"(</div>
    </body>
    </html>
    )";

	response.setBody(std::move(page));
	response.setContentType("text/html; charset=utf-8");
	return response;
}
