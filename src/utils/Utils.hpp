#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <ctime>

class Utils {
public:
    static std::string trim(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::string toLowerCase(const std::string& str);
    static std::string toUpperCase(const std::string& str);
    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);
    
    static bool fileExists(const std::string& path);
    static std::string readFile(const std::string& path);
    static bool isDirectory(const std::string& path);
    
    // Network util
    static std::string intToString(int value);
    static int stringToInt(const std::string& str);
    
    // Time util
    static std::string getCurrentTimestamp();
    static std::string formatHttpDate(time_t timestamp);
    
    static std::string urlDecode(const std::string& str);
    
private:
    Utils();
};

#endif