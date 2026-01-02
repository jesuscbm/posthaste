#include "utils.hpp"
#include <cstddef>
#include <ctime>
#include <filesystem>
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
	if (id.length() < 4)
		return;

	if (id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
		!= std::string::npos) {
		return;
	}

	if (!std::filesystem::is_directory("p/"))
		std::filesystem::create_directory("p/");

	std::string shard1 = "p/" + id.substr(0, 1) + "/";
	std::string shard2 = shard1 + id.substr(1, 1) + "/";
	std::filesystem::create_directory(shard1);
	std::filesystem::create_directory(shard2);
	std::ofstream output(shard2 + id.substr(2));
	std::ofstream metadata(shard2 + id.substr(2) + ".meta");

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

std::string html_escape(const std::string_view &data)
{
	std::string buffer;

	// Spare space to minimize reallocations
	buffer.reserve(data.size() + (data.size() >> 3));

	size_t pos = 0;
	while (pos < data.size()) {
		size_t next = data.find_first_of("&\"'<>", pos);
		if (next == std::string_view::npos) {
			buffer.append(data.substr(pos));
			break;
		}

		buffer.append(data.substr(pos, next - pos));

		switch (data[next]) {
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
		}

		pos = next + 1;
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
