#pragma once

#include <vector>
#include <string>

inline std::vector<std::string> split_string(const std::string& in, const std::string& delimiter) {
    std::vector<std::string> result{};
    size_t last{0}, next;
    while ((next = in.find(delimiter, last)) != std::string::npos) {
        result.push_back(in.substr(last, next - last));
        last = next + delimiter.size();
    }
    result.push_back(in.substr(last));
    return result;
}

inline std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ") {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

inline std::string ltrim(std::string&& str, const std::string& chars = "\t\n\v\f\r ") {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

inline std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ") {
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

inline std::string rtrim(std::string&& str, const std::string& chars = "\t\n\v\f\r ") {
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

inline std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ") {
    return ltrim(rtrim(str, chars), chars);
}

inline std::string trim(std::string&& str, const std::string& chars = "\t\n\v\f\r ") {
    return ltrim(rtrim(str, chars), chars);
}