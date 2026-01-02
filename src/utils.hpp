#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>

#include <string_view>
#include <unordered_map>

std::unordered_map<std::string, std::string> parse_form_data(const std::string &body);
std::string url_decode(const std::string &src);
std::string generate_id(int length = 6);
void save_paste_to_disk(const std::string &id, const std::string &content,
						const std::string &expiration);
std::string html_escape(const std::string_view &data);

#endif
