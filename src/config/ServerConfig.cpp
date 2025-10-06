#include "ServerConfig.hpp"
#include <algorithm>
#include "Logger.hpp"

LocationConfig::LocationConfig() 
    : autoindex(false), cgi_enabled(false) {
}

ServerConfig::ServerConfig() : _port(8080), _host("127.0.0.1"), _serverName("localhost"), _clientMaxBodySize(1048576) {
    // Default error pages
    _errorPages[404] = "./errors/404.html";
    _errorPages[500] = "./errors/500.html";
    _errorPages[403] = "./errors/403.html";
}

ServerConfig::~ServerConfig() {
}

int ServerConfig::getPort() const {
    return _port;
}

const std::string& ServerConfig::getHost() const {
    return _host;
}

const std::string& ServerConfig::getServerName() const {
    return _serverName;
}

size_t ServerConfig::getClientMaxBodySize() const {
    return _clientMaxBodySize;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
    return _locations;
}

std::string ServerConfig::getErrorPage(int errorCode) const {
    std::map<int, std::string>::const_iterator it = _errorPages.find(errorCode);
    if (it != _errorPages.end())
        return it->second;
    return "";
}

void ServerConfig::setPort(int port) {
    _port = port;
}

void ServerConfig::setHost(const std::string& host) {
    _host = host;
}

void ServerConfig::setServerName(const std::string& serverName) {
    _serverName = serverName;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
    _clientMaxBodySize = size;
}

void ServerConfig::addLocation(const LocationConfig& location) {
    _locations.push_back(location);
}

void ServerConfig::addErrorPage(int errorCode, const std::string& path) {
    _errorPages[errorCode] = path;
}

const LocationConfig* ServerConfig::findLocation(const std::string& path) const {
    const LocationConfig* bestMatch = NULL;
    size_t bestMatchLength = 0;
    
    for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); 
         it != _locations.end(); ++it) {
        
        // Pour éviter le faux matching, vérifie que c'est un match de segment complet
        if (path == it->path || 
            (path.find(it->path) == 0 && 
             (it->path == "/" || path[it->path.length()] == '/'))) {
            
            if (it->path.length() > bestMatchLength) {
                bestMatch = &(*it);
                bestMatchLength = it->path.length();
            }
        }
    }
    
    return bestMatch;
}

bool ServerConfig::isMethodAllowed(const std::string& path, const std::string& method) const {
    const LocationConfig* location = findLocation(path);
    if (!location) {
        Logger::debug("No location found for path: " + path);
        return false;
    }
    
    Logger::debug("Found location '" + location->path + "' for path: " + path);
    Logger::debug("Allowed methods for this location:");
    for (size_t i = 0; i < location->methods.size(); ++i) {
        Logger::debug("  - " + location->methods[i]);
    }
    
    bool allowed = std::find(location->methods.begin(), location->methods.end(), method) 
                   != location->methods.end();
    Logger::debug("Method '" + method + "' allowed: " + (allowed ? "YES" : "NO"));
    
    return allowed;
}

// bool ServerConfig::isMethodAllowed(const std::string& path, const std::string& method) const {
//     const LocationConfig* location = findLocation(path);
//     if (!location)
//         return false;
    
//     return std::find(location->methods.begin(), location->methods.end(), method) 
// //           != location->methods.end();
// }