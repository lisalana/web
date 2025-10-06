#ifndef FILESERVER_HPP
#define FILESERVER_HPP

#include <string>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"

class FileServer {
public:
    static HTTPResponse serveFile(const HTTPRequest& request, const ServerConfig& config);
    static HTTPResponse serveStaticFile(const std::string& filepath);
    static HTTPResponse serveDirectory(const std::string& dirpath, const std::string& uri, bool autoindex);
    static HTTPResponse createErrorResponse(int statusCode, const std::string& message = "");
    static HTTPResponse handleDelete(const HTTPRequest& request, const ServerConfig& config);
    
private:
    static std::string resolveFilePath(const std::string& uri, const LocationConfig& location);
    static bool isValidPath(const std::string& path);
    static bool pathExists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool isReadable(const std::string& path);
    
    // Directory listing
    static std::string generateDirectoryListing(const std::string& dirpath, const std::string& uri);
    static std::vector<std::string> getDirectoryEntries(const std::string& dirpath);
    
    // Security
    static bool isPathTraversalAttempt(const std::string& path);
    static std::string sanitizePath(const std::string& path);
    
    // Error pages
    static HTTPResponse loadErrorPage(int statusCode, const ServerConfig& config);
    static std::string getDefaultErrorPage(int statusCode, const std::string& message = "");
};

#endif