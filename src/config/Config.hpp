#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <string>
#include <vector>
#include "ServerConfig.hpp"

class Config {
private:
    std::vector<ServerConfig> _servers;
    std::string _configFile;

public:
    Config();
    Config(const std::string& configFile);
    ~Config();

    // Parse configuration file (original method)
    bool parseFile(const std::string& configFile);
    
    // New methods for different formats
    bool parseNginx(const std::string& configFile);
    bool parseYaml(const std::string& configFile);

    // Getters
    const std::vector<ServerConfig>& getServers() const;
    ServerConfig* getServerByPort(int port);
    const ServerConfig* getServerByHostPort(const std::string& host, int port) const;

    // Utils
    bool isValid() const;
    size_t getServerCount() const;

private:
    // Nginx parsing methods (your existing ones)
    bool parseServerBlock(const std::vector<std::string>& lines, size_t& index, ServerConfig& server);
    bool parseLocationBlock(const std::vector<std::string>& lines, size_t& index, LocationConfig& location);
    std::string extractValue(const std::string& line);
    std::vector<std::string> extractMethods(const std::string& line);
    bool isBlockStart(const std::string& line, const std::string& blockType);
    bool isBlockEnd(const std::string& line);
    std::string trim(const std::string& str);
    
    // YAML parsing methods
    bool parseYamlServer(const std::string& content, ServerConfig& server);
    std::string getYamlValue(const std::string& content, const std::string& key, size_t& pos);
    bool parseYamlLocations(const std::string& content, ServerConfig& server);
    bool parseYamlLocation(const std::string& content, const std::string& path, size_t& pos, LocationConfig& location);
    size_t findYamlSection(const std::string& content, const std::string& key, size_t startPos = 0);
    int getIndentLevel(const std::string& line);
};

#endif