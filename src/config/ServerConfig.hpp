#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    std::vector<std::string> methods;
    std::string uploadPath;
    bool autoindex;
    bool cgi_enabled;
    std::string cgi_extension;  // (.php, .py)
    std::string cgi_path;       // (/usr/bin/php-cgi)
    std::string redirect;  // Format: "301 /new-path" ou "302 /other-path"
    
    LocationConfig();
};

class ServerConfig {
private:
    int _port;
    std::string _host;
    std::string _serverName;
    size_t _clientMaxBodySize;
    std::map<int, std::string> _errorPages;
    std::vector<LocationConfig> _locations;

public:
    ServerConfig();
    ~ServerConfig();
    
    // Getters
    int getPort() const;
    const std::string& getHost() const;
    const std::string& getServerName() const;
    size_t getClientMaxBodySize() const;
    const std::vector<LocationConfig>& getLocations() const;
    std::string getErrorPage(int errorCode) const;
    
    // Setters
    void setPort(int port);
    void setHost(const std::string& host);
    void setServerName(const std::string& serverName);
    void setClientMaxBodySize(size_t size);
    void addLocation(const LocationConfig& location);
    void addErrorPage(int errorCode, const std::string& path);
    
    // Utils
    const LocationConfig* findLocation(const std::string& path) const;
    bool isMethodAllowed(const std::string& path, const std::string& method) const;
};

#endif