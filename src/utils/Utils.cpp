#include "Utils.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <ctime>

std::string Utils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string Utils::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::toUpperCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool Utils::startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length())
        return false;
    return str.substr(0, prefix.length()) == prefix;
}

bool Utils::endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length())
        return false;
    return str.substr(str.length() - suffix.length()) == suffix;
}

bool Utils::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string Utils::readFile(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return "";
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    file.close();
    return content;
}

bool Utils::isDirectory(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    return S_ISDIR(buffer.st_mode);
}

std::string Utils::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

int Utils::stringToInt(const std::string& str) {
    std::stringstream ss(str);
    int value;
    ss >> value;
    return value;
}

std::string Utils::getCurrentTimestamp() {
    time_t now = time(0);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buffer);
}

std::string Utils::formatHttpDate(time_t timestamp) {
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&timestamp));
    return std::string(buffer);
}

std::string Utils::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i+1, 2);
            int value = 0;
            std::stringstream ss;
            ss << std::hex << hex;
            ss >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            result += ' '; // + devient espace en URL encoding
        } else {
            result += str[i];
        }
    }
    return result;
}