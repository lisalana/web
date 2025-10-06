#include "Config.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
 #include <cstdlib>

Config::Config() {
}

Config::Config(const std::string& configFile) : _configFile(configFile) {
    parseFile(configFile);
}

Config::~Config() {
}

bool Config::parseFile(const std::string& configFile) {
    _configFile = configFile;
    _servers.clear();

    if (!Utils::fileExists(configFile)) {
        Logger::error("Config file not found: " + configFile);
        return false;
    }

    std::string content = Utils::readFile(configFile);
    if (content.empty()) {
        Logger::error("Config file is empty or cannot be read: " + configFile);
        return false;
    }

    // Split content into lines
    std::vector<std::string> lines = Utils::split(content, '\n');
    
    // Remove empty lines and comments
    std::vector<std::string> cleanLines;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = Utils::trim(lines[i]);
        if (!line.empty() && line[0] != '#') {
            cleanLines.push_back(line);
        }
    }

    // Parse server blocks
    for (size_t i = 0; i < cleanLines.size(); ++i) {
        if (isBlockStart(cleanLines[i], "server")) {
            ServerConfig server;
            ++i; // Skip opening brace
            if (parseServerBlock(cleanLines, i, server)) {
                _servers.push_back(server);
                Logger::info("Parsed server config for port " + Utils::intToString(server.getPort()));
            } else {
                Logger::error("Failed to parse server block");
                return false;
            }
        }
    }

    if (_servers.empty()) {
        Logger::warning("No server configuration found, using default");
        ServerConfig defaultServer;
        LocationConfig defaultLocation;
        defaultLocation.path = "/";
        defaultLocation.root = "./static";
        defaultLocation.index = "index.html";
        defaultLocation.methods.push_back("GET");
        defaultLocation.methods.push_back("POST");
        defaultServer.addLocation(defaultLocation);
        _servers.push_back(defaultServer);
    }

    return true;
}

bool Config::parseServerBlock(const std::vector<std::string>& lines, size_t& index, ServerConfig& server) {
    while (index < lines.size()) {
        std::string line = Utils::trim(lines[index]);
        
        if (isBlockEnd(line)) {
            return true;
        }
        
        if (Utils::startsWith(line, "listen")) {
            int port = Utils::stringToInt(extractValue(line));
            if (port > 0 && port <= 65535) {
                server.setPort(port);
            }
        }
        else if (Utils::startsWith(line, "host")) {
            server.setHost(extractValue(line));
        }
        else if (Utils::startsWith(line, "server_name")) {
            server.setServerName(extractValue(line));
        }
        else if (Utils::startsWith(line, "client_max_body_size")) {
            std::string value = extractValue(line);
            size_t size;
    
            if (Utils::endsWith(value, "M") || Utils::endsWith(value, "m")) {
                std::string number = value.substr(0, value.length()-1);
                size = Utils::stringToInt(number) * 1048576;
            } 
            else {
                size = Utils::stringToInt(value);
            }
    
            server.setClientMaxBodySize(size);
            Logger::debug("Set max body size to: " + Utils::intToString(size) + " bytes");
        }
        // else if (Utils::startsWith(line, "client_max_body_size")) {
        //     std::string value = extractValue(line);
        //     size_t size = Utils::stringToInt(value);
        //     // Handle M suffix for megabytes
        //     if (Utils::endsWith(value, "M") || Utils::endsWith(value, "m")) {
        //         size *= 1048576; // Convert MB to bytes
        //     }
        //     server.setClientMaxBodySize(size);
        // }
        else if (isBlockStart(line, "location")) {
            LocationConfig location;
            // Extract location path
            size_t start = line.find_first_of(" \t");
            size_t end = line.find_last_of("{");
            if (start != std::string::npos && end != std::string::npos) {
                location.path = Utils::trim(line.substr(start, end - start));
            }
            
            ++index; // Skip opening brace
            if (parseLocationBlock(lines, index, location)) {
                server.addLocation(location);
            }
        }
        
        ++index;
    }
    return false;
}

bool Config::parseLocationBlock(const std::vector<std::string>& lines, size_t& index, LocationConfig& location) {
    while (index < lines.size()) {
        std::string line = Utils::trim(lines[index]);
        
        if (isBlockEnd(line)) {
            return true;
        }
        
        if (Utils::startsWith(line, "root")) {
            location.root = extractValue(line);
        }
        else if (Utils::startsWith(line, "index")) {
            location.index = extractValue(line);
        }
        else if (Utils::startsWith(line, "methods")) {
            location.methods = extractMethods(line);
        }
        else if (Utils::startsWith(line, "upload_path")) {
            location.uploadPath = extractValue(line);
        }
        else if (Utils::startsWith(line, "autoindex")) {
            std::string value = Utils::toLowerCase(extractValue(line));
            location.autoindex = (value == "on" || value == "true" || value == "yes");
        }
        else if (Utils::startsWith(line, "cgi_extension")) {
            location.cgi_extension = extractValue(line);
            location.cgi_enabled = true;
        }
        else if (Utils::startsWith(line, "cgi_path")) {
            location.cgi_path = extractValue(line);
        }

        else if (Utils::startsWith(line, "return")) {
            location.redirect = extractValue(line);
            Logger::debug("Parsed redirect: " + location.redirect + " for location: " + location.path);
        }
        
        ++index;
    }
    return false;
}

std::string Config::extractValue(const std::string& line) {
    size_t pos = line.find_first_of(" \t");
    if (pos == std::string::npos) return "";
    
    std::string value = Utils::trim(line.substr(pos));
    // Remove trailing semicolon if present
    if (!value.empty() && value[value.length()-1] == ';') {
        value = value.substr(0, value.length()-1);
    }
    return value;
}

std::vector<std::string> Config::extractMethods(const std::string& line) {
    std::string value = extractValue(line);
    return Utils::split(value, ' ');
}

bool Config::isBlockStart(const std::string& line, const std::string& blockType) {
    return Utils::startsWith(line, blockType) && line.find("{") != std::string::npos;
}

bool Config::isBlockEnd(const std::string& line) {
    return line == "}";
}

std::string Config::trim(const std::string& str) {
    return Utils::trim(str);
}

const std::vector<ServerConfig>& Config::getServers() const {
    return _servers;
}

ServerConfig* Config::getServerByPort(int port) {
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].getPort() == port) {
            return &_servers[i];
        }
    }
    return NULL;
}

const ServerConfig* Config::getServerByHostPort(const std::string& host, int port) const {
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].getHost() == host && _servers[i].getPort() == port) {
            return &_servers[i];
        }
    }
    return NULL;
}

bool Config::isValid() const {
    return !_servers.empty();
}

size_t Config::getServerCount() const {
    return _servers.size();
}


bool Config::parseNginx(const std::string& configFile)
{
    return parseFile(configFile);
}

bool Config::parseYaml(const std::string& configFile)
{
    std::ifstream file(configFile.c_str());
    if (!file.is_open()) {
        return false;
    }

    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    file.close();

    _configFile = configFile;
    _servers.clear();

    // Parse server block
    ServerConfig server;
    if (!parseYamlServer(content, server)) {
        return false;
    }

    _servers.push_back(server);
    return true;
}

bool Config::parseYamlServer(const std::string& content, ServerConfig& server)
{
    size_t pos = 0;
    
    std::string listen = getYamlValue(content, "listen", pos);
    if (!listen.empty()) {
        server.setPort(atoi(listen.c_str()));
    }

    pos = 0;
    std::string host = getYamlValue(content, "host", pos);
    if (!host.empty()) {
        server.setHost(host);
    }

    pos = 0;
    std::string serverName = getYamlValue(content, "server_name", pos);
    if (!serverName.empty()) {
        server.setServerName(serverName);
    }

    pos = 0;
    std::string maxBodySize = getYamlValue(content, "client_max_body_size", pos);
    if (!maxBodySize.empty()) {
        server.setClientMaxBodySize(atoi(maxBodySize.c_str()));
    }

    pos = 0;
    size_t errorPagesPos = findYamlSection(content, "error_pages", 0);
    if (errorPagesPos != std::string::npos) {
        size_t lineStart = errorPagesPos;
        size_t lineEnd = content.find('\n', lineStart);
        
        while (lineEnd != std::string::npos) {
            lineStart = lineEnd + 1;
            lineEnd = content.find('\n', lineStart);
            if (lineEnd == std::string::npos) lineEnd = content.length();
            
            std::string line = content.substr(lineStart, lineEnd - lineStart);
            line = trim(line);
            
            if (line.empty() || getIndentLevel(line) < 2) {
                break;
            }

            // Parse error page entry (e.g., "400: ./errors/400.html")
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string code = trim(line.substr(0, colonPos));
                std::string path = trim(line.substr(colonPos + 1));
                server.addErrorPage(atoi(code.c_str()), path);
            }
        }
    }

    if (!parseYamlLocations(content, server)) {
        return false;
    }

    return true;
}
bool Config::parseYamlLocations(const std::string& content, ServerConfig& server)
{
    size_t locationsPos = findYamlSection(content, "locations", 0);
    if (locationsPos == std::string::npos) {
        return true;
    }
    
    size_t pos = locationsPos;
    size_t lineStart = pos;
    size_t lineEnd = content.find('\n', lineStart);
    
    while (lineEnd != std::string::npos) {
        lineStart = lineEnd + 1;
        if (lineStart >= content.length()) break; // Protection overflow
        
        lineEnd = content.find('\n', lineStart);
        if (lineEnd == std::string::npos) lineEnd = content.length();
        
        if (lineStart >= content.length() || lineEnd > content.length()) break; // Double protection
        
        std::string line = content.substr(lineStart, lineEnd - lineStart);
        line = trim(line);
        
        if (line.empty() || getIndentLevel(content.substr(lineStart, lineEnd - lineStart)) < 2) {
            break;
        }

        if (!line.empty() && line[line.size() - 1] == ':') {
            std::string locationPath = line.substr(0, line.length() - 1);
            locationPath = trim(locationPath);
            
            if (!locationPath.empty() && locationPath[0] == '/') {
                LocationConfig location;
                location.path = locationPath;
                
                size_t locationContentPos = lineStart;
                if (parseYamlLocation(content, locationPath, locationContentPos, location)) {
                    server.addLocation(location);
                }
            }
        }
    }
    
    return true;
}

bool Config::parseYamlLocation(const std::string& content, const std::string& path, size_t& pos, LocationConfig& location)
{
    location.path = path;
    
    size_t lineStart = pos;
    size_t lineEnd = content.find('\n', lineStart);
    
    while (lineEnd != std::string::npos) {
        lineStart = lineEnd + 1;
        if (lineStart >= content.length()) break; // Protection overflow
        
        lineEnd = content.find('\n', lineStart);
        if (lineEnd == std::string::npos) lineEnd = content.length();
        
        if (lineStart >= content.length() || lineEnd > content.length()) break; // Double protection
        
        std::string line = content.substr(lineStart, lineEnd - lineStart);
        std::string originalLine = line;
        line = trim(line);
        
        if (line.empty()) continue;
        
        int indent = getIndentLevel(originalLine);
        if (indent < 6) {
            break;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));

            if (key == "root") {
                location.root = value;
            } else if (key == "index") {
                location.index = value;
            } else if (key == "autoindex") {
                location.autoindex = (value == "true");
            } else if (key == "upload_path") {
                location.uploadPath = value;
            } else if (key == "cgi_extension") {
                location.cgi_extension = value;
                location.cgi_enabled = true;
            } else if (key == "cgi_path") {
                location.cgi_path = value;
            } else if (key == "methods") {
                location.methods.clear();
                while (lineEnd != std::string::npos) {
                    lineStart = lineEnd + 1;
                    if (lineStart >= content.length()) break;
                    
                    lineEnd = content.find('\n', lineStart);
                    if (lineEnd == std::string::npos) lineEnd = content.length();
                    
                    if (lineStart >= content.length() || lineEnd > content.length()) break;
                    
                    std::string methodLine = content.substr(lineStart, lineEnd - lineStart);
                    std::string originalMethodLine = methodLine;
                    methodLine = trim(methodLine);
                    
                    if (methodLine.empty()) continue;
                    
                    int methodIndent = getIndentLevel(originalMethodLine);
                    if (methodIndent < 8) {
                        lineEnd = lineStart - 1;
                        break;
                    }
                    
                    if (methodLine.find("- ") == 0) {
                        std::string method = trim(methodLine.substr(2));
                        location.methods.push_back(method);
                    }
                }
            }
        }
    }

    pos = lineEnd;
    return true;
}

std::string Config::getYamlValue(const std::string& content, const std::string& key, size_t& pos)
{
    size_t keyPos = content.find(key + ":", pos);
    if (keyPos == std::string::npos) {
        return "";
    }

    size_t valueStart = keyPos + key.length() + 1;
    size_t valueEnd = content.find('\n', valueStart);
    if (valueEnd == std::string::npos) {
        valueEnd = content.length();
    }

    std::string value = content.substr(valueStart, valueEnd - valueStart);
    return trim(value);
}

size_t Config::findYamlSection(const std::string& content, const std::string& key, size_t startPos)
{
    return content.find(key + ":", startPos);
}

int Config::getIndentLevel(const std::string& line)
{
    int indent = 0;
    for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == ' ') {
            indent++;
        } else if (line[i] == '\t') {
            indent += 4; // Count tab as 4 spaces
        } else {
            break;
        }
    }
    return indent;
}