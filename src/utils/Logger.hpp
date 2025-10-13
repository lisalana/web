#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <ctime>

enum LogLevel 
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger 
{
private:
    static LogLevel _level;
    
public:
    static void setLevel(LogLevel level);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void info(const std::string& message, int fd);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    
private:
    static void log(LogLevel level, const std::string& message, int fd = -1);
    static std::string getCurrentTime();
    static std::string levelToString(LogLevel level);
};

#endif
