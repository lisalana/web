/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rguigneb <rguigneb@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/13 16:22:44 by rguigneb          #+#    #+#             */
/*   Updated: 2025/10/13 16:22:45 by rguigneb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"
#include "Color.hpp"

LogLevel Logger::_level = INFO;

void Logger::setLevel(LogLevel level) {
    _level = level;
}

void Logger::debug(const std::string& message) {
    if (_level <= DEBUG)
        log(DEBUG, message);
}

void Logger::info(const std::string& message) {
    if (_level <= INFO)
        log(INFO, message);
}

void Logger::info(const std::string& message, int fd) {
    if (_level <= INFO)
        log(INFO, message, fd);
}

void Logger::warning(const std::string& message) {
    if (_level <= WARNING)
        log(WARNING, message);
}

void Logger::error(const std::string& message) {
    if (_level <= ERROR)
        log(ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message, int fd) {
    Color levelColor = Colors::WHITE;
    
    switch (level) {
        case DEBUG:   levelColor = Colors::GRAY; break;
        case INFO:    levelColor = Colors::CYAN; break;
        case WARNING: levelColor = Colors::YELLOW; break;
        case ERROR:   levelColor = Colors::RED; break;
    }
    
    std::cout << Colors::GRAY << "[" << getCurrentTime() << "] " 
              << levelColor << "[" << levelToString(level) << "] "
              << Colors::WHITE;

    if (fd >= 0) {
        std::cout << Colors::MAGENTA << "[" << fd << "] " << Colors::WHITE;
    }

    std::cout << message << TextFormat::RESET << std::endl;
}

std::string Logger::getCurrentTime() {
    time_t now = time(0);
    char* timeStr = ctime(&now);
    std::string result(timeStr);
    // Remove newline
    if (!result.empty() && result[result.length()-1] == '\n')
        result.erase(result.length()-1);
    return result;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case DEBUG:   return "DEBUG";
        case INFO:    return "INFO";
        case WARNING: return "WARN";
        case ERROR:   return "ERROR";
        default:      return "UNKNOWN";
    }
}
