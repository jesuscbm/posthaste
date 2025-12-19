#include "utils.hpp"
#include <ctime>
#include <fstream>
#include <random>
#include <sstream>

std::string url_decode(const std::string &src)
{
	std::string ret;
	char ch;
	size_t i;
	int ii;
	for (i = 0; i < src.length(); i++) {
		if (src[i] == '%') {
			sscanf(src.substr(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		} else if (src[i] == '+') {
			ret += ' ';
		} else {
			ret += src[i];
		}
	}
	return ret;
}

std::unordered_map<std::string, std::string> parse_form_data(const std::string &body)
{
	std::unordered_map<std::string, std::string> data;
	std::stringstream ss(body);
	std::string pair;

	while (std::getline(ss, pair, '&')) {
		size_t pos = pair.find('=');
		if (pos != std::string::npos) {
			std::string key = pair.substr(0, pos);
			std::string val = url_decode(pair.substr(pos + 1));
			data[key] = val;
		}
	}
	return data;
}

void save_paste_to_disk(const std::string &id, const std::string &content,
						const std::string &expiry)
{
	std::ofstream output("p/" + id);
	std::ofstream metadata("p/" + id + ".meta");

	long long expiry_timestamp = -1;
	std::time_t now = std::time(nullptr);

	if (expiry == "1h") {
		expiry_timestamp = now + 3600;	// +1 hour
	} else if (expiry == "1d") {
		expiry_timestamp = now + 86400;	 // +24 hours
	} else if (expiry == "1w") {
		expiry_timestamp = now + 604800;  // +7 days
	}
	metadata << expiry_timestamp;
	output << content;

	output.close();
	metadata.close();
}

std::string html_escape(const std::string &data)
{
	std::string buffer;
	buffer.reserve(data.size());
	for (size_t pos = 0; pos != data.size(); ++pos) {
		switch (data[pos]) {
		case '&':
			buffer.append("&amp;");
			break;
		case '\"':
			buffer.append("&quot;");
			break;
		case '\'':
			buffer.append("&apos;");
			break;
		case '<':
			buffer.append("&lt;");
			break;
		case '>':
			buffer.append("&gt;");
			break;
		default:
			buffer.append(&data[pos], 1);
			break;
		}
	}
	return buffer;
}

std::string generate_id(int length)
{
	const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	// Generador de números aleatorios moderno
	std::random_device rd;
	std::mt19937 generator(rd());
	std::uniform_int_distribution<> distribution(0, charset.size() - 1);

	std::string id;
	for (int i = 0; i < length; ++i) {
		id += charset[distribution(generator)];
	}

	return id;
}
