

#include "FileServer.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "PostHandler.hpp"

HTTPResponse FileServer::serveFile(const HTTPRequest& request, const ServerConfig& config) {
    
    // Check for stop server request
    if (request.getURI() == "/stop") {
        Logger::info("Stop server request received");
        
        HTTPResponse response(200);
        std::string stopHTML = 
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head>\n"
            "    <meta charset='UTF-8'>\n"
            "    <title>Server Stopping</title>\n"
            "    <style>\n"
            "        body { font-family: -apple-system, sans-serif; text-align: center; padding: 50px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }\n"
            "        .container { max-width: 500px; margin: 0 auto; background: white; border-radius: 20px; padding: 40px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); }\n"
            "        h1 { color: #dc2626; margin-bottom: 20px; }\n"
            "        p { color: #4a5568; line-height: 1.6; }\n"
            "    </style>\n"
            "</head>\n"
            "<body>\n"
            "    <div class='container'>\n"
            "        <h1>🛑 Server Shutting Down</h1>\n"
            "        <p>The server is stopping gracefully...</p>\n"
            "        <p>Goodbye! 👋</p>\n"
            "    </div>\n"
            "</body>\n"
            "</html>";
        
        response.setBody(stopHTML);
        response.setContentType("text/html");
        response.addHeader("Connection", "close");
        response.setShouldStopServer(true);
        
        return response;
    }
    
    // Find matching location
    const LocationConfig* location = config.findLocation(request.getURI());
    if (!location) {
        Logger::debug("No location found for URI: " + request.getURI());
        return createErrorResponse(404);
    }

     // Verification de redirection
    if (!location->redirect.empty()) {
        HTTPResponse response;
        
        // Parse "301 /new-path"
        std::istringstream iss(location->redirect);
        int status_code;
        std::string new_path;
        iss >> status_code >> new_path;
        
        response.setStatusCode(status_code);
        response.addHeader("Location", new_path);
        response.setBody("");
        
        Logger::info("Redirect " + Utils::intToString(status_code) + ": " + request.getURI() + " -> " + new_path);
        return response;
    }

    // Check if method is allowed
    Logger::debug("Checking method: " + request.methodToString() + " for URI: " + request.getURI());
    if (!config.isMethodAllowed(request.getURI(), request.methodToString())) {
        Logger::debug("Method not allowed: " + request.methodToString() + " for " + request.getURI());
        return createErrorResponse(405);
    }
    
    if (request.getMethod() == METHOD_POST) {
        return PostHandler::handlePost(request, config);
    }
    
    if (request.getMethod() == METHOD_DELETE) {
        return handleDelete(request, config);
    }

    // Resolve file path for GET requests
    std::string filepath = resolveFilePath(request.getURI(), *location);
    
    // Security check for path traversal
    if (isPathTraversalAttempt(filepath)) {
        Logger::warning("Path traversal attempt detected: " + filepath);
        return createErrorResponse(403);
    }
    
    filepath = sanitizePath(filepath);
    
    if (!pathExists(filepath)) {
        Logger::debug("File not found: " + filepath);
        return loadErrorPage(404, config);
    }
    
    if (isDirectory(filepath)) {
        // Try to serve index file
        if (!location->index.empty()) {
            std::string indexPath = filepath + "/" + location->index;
            if (pathExists(indexPath) && !isDirectory(indexPath)) {
                return serveStaticFile(indexPath);
            }
        }
        
        // Directory listing if enabled
        if (location->autoindex) {
            return serveDirectory(filepath, request.getURI(), true);
        } else {
            Logger::debug("Directory listing disabled for: " + filepath);
            return createErrorResponse(403);
        }
    }
    return serveStaticFile(filepath);
}

HTTPResponse FileServer::handleDelete(const HTTPRequest& request, const ServerConfig& config) {
    Logger::debug("Processing DELETE request for: " + request.getURI());
    
    const LocationConfig* location = config.findLocation(request.getURI());
    if (!location) {
        return createErrorResponse(404);
    }
    std::string filepath = resolveFilePath(request.getURI(), *location);
    
    // Security check for path traversal
    if (isPathTraversalAttempt(filepath)) {
        Logger::warning("DELETE: Path traversal attempt detected: " + filepath);
        return createErrorResponse(403);
    }
    
    filepath = sanitizePath(filepath);
    filepath = Utils::urlDecode(filepath);
    
    if (!pathExists(filepath)) {
        Logger::debug("DELETE: File not found: " + filepath);
        return createErrorResponse(404);
    }
    
    if (isDirectory(filepath)) {
        Logger::debug("DELETE: Cannot delete directory: " + filepath);
        return createErrorResponse(403);
    }
    
    // Try to delete the file
    if (unlink(filepath.c_str()) == 0) {
        Logger::info("File deleted successfully: " + filepath);
        
        // Return success response
        HTTPResponse response(200);
        std::string successHTML = 
            "<!DOCTYPE html>\n"
            "<html><head><title>File Deleted</title></head><body>\n"
            "<h1>File Deleted Successfully</h1>\n"
            "<p>The file has been deleted.</p>\n"
            "<a href=\"/\">Back to home</a>\n"
            "</body></html>";
        response.setBody(successHTML);
        response.setContentType("text/html");
        return response;
    } else {
        Logger::error("Failed to delete file: " + filepath);
        return createErrorResponse(500);
    }
}

HTTPResponse FileServer::serveStaticFile(const std::string& filepath) {
    if (!isReadable(filepath)) {
        Logger::debug("File not readable: " + filepath);
        return createErrorResponse(403);
    }
    
    std::string content = Utils::readFile(filepath);
    if (content.empty()) {
        Logger::error("Failed to read file: " + filepath);
        return createErrorResponse(500);
    }
    
    HTTPResponse response(200);
    response.setBody(content);
    response.setContentType(HTTPResponse::getContentTypeByExtension(filepath));
    
    Logger::debug("Served file: " + filepath + " (" + Utils::intToString(content.length()) + " bytes)");
    return response;
}

HTTPResponse FileServer::serveDirectory(const std::string& dirpath, const std::string& uri, bool autoindex) {
    if (!autoindex) {
        return createErrorResponse(403);
    }
    
    std::string html = generateDirectoryListing(dirpath, uri);
    
    HTTPResponse response(200);
    response.setBody(html);
    response.setContentType("text/html; charset=UTF-8");
    
    Logger::debug("Generated directory listing for: " + dirpath);
    return response;
}

HTTPResponse FileServer::createErrorResponse(int statusCode, const std::string& message) {
    HTTPResponse response(statusCode);
    std::string errorMessage = message.empty() ? HTTPResponse::getStatusMessage(statusCode) : message;
    std::string html = getDefaultErrorPage(statusCode, errorMessage);
    response.setBody(html);
    return response;
}

std::string FileServer::resolveFilePath(const std::string& uri, const LocationConfig& location) {
    std::string path = location.root;
    
    // Remove leading slash from URI for path joining
    std::string relative_uri = uri;
    if (!relative_uri.empty() && relative_uri[0] == '/') {
        relative_uri = relative_uri.substr(1);
    }
    
    // Remove location path prefix from URI
    if (!location.path.empty() && location.path != "/") {
        std::string loc_path = location.path;
        if (loc_path[0] == '/') {
            loc_path = loc_path.substr(1);
        }
        if (relative_uri.find(loc_path) == 0) {
            relative_uri = relative_uri.substr(loc_path.length());
            if (!relative_uri.empty() && relative_uri[0] == '/') {
                relative_uri = relative_uri.substr(1);
            }
        }
    }
    
    // Join paths
    if (!path.empty() && path[path.length()-1] != '/' && !relative_uri.empty()) {
        path += "/";
    }
    path += relative_uri;
    
    return path;
}

bool FileServer::isValidPath(const std::string& path) {
    return !path.empty() && path.find("..") == std::string::npos;
}

bool FileServer::pathExists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool FileServer::isDirectory(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool FileServer::isReadable(const std::string& path) {
    return access(path.c_str(), R_OK) == 0;
}

std::string FileServer::generateDirectoryListing(const std::string& path, const std::string& uri) {
    std::string html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Directory: " + uri + "</title>";
    html += "<style>";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
    html += "body { font-family: -apple-system, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 40px 20px; }";
    html += ".container { max-width: 900px; margin: 0 auto; background: white; border-radius: 20px; padding: 40px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); }";
    html += "h1 { color: #2d3748; margin-bottom: 10px; }";
    html += ".path { color: #718096; margin-bottom: 30px; font-size: 0.9em; }";
    html += ".item { padding: 15px; margin: 8px 0; background: #f7fafc; border-radius: 8px; display: flex; justify-content: space-between; align-items: center; transition: all 0.2s; }";
    html += ".item:hover { background: #edf2f7; transform: translateX(5px); }";
    html += ".item a { color: #667eea; text-decoration: none; font-weight: 500; }";
    html += ".item a:hover { color: #764ba2; }";
    html += ".folder { color: #4299e1; }";
    html += ".file { color: #48bb78; }";
    html += ".delete-btn { background: #f56565; color: white; border: none; padding: 8px 16px; border-radius: 6px; cursor: pointer; font-weight: 600; }";
    html += ".delete-btn:hover { background: #e53e3e; }";
    html += ".back-link { display: inline-block; margin-top: 20px; padding: 12px 24px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; text-decoration: none; border-radius: 8px; font-weight: 600; }";
    html += ".back-link:hover { transform: translateY(-2px); }";
    html += ".footer { text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e2e8f0; color: #a0aec0; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>📁 Directory Listing</h1>";
    html += "<p class='path'>" + uri + "</p>";
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        html += "<p style='color: #e53e3e;'>Error opening directory</p></div></body></html>";
        return html;
    }
    
    struct dirent* entry;
    std::vector<std::pair<std::string, bool> > items; // pair<name, isDir>
    
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        
        std::string fullPath = path + "/" + name;
        bool isDir = Utils::isDirectory(fullPath);
        items.push_back(std::make_pair(name, isDir));
    }
    closedir(dir);
    
    // Sort: directories first, then files
    std::sort(items.begin(), items.end());
    
    for (size_t i = 0; i < items.size(); ++i) {
        std::string name = items[i].first;
        bool isDir = items[i].second;
        std::string linkUri = uri;
        if (linkUri[linkUri.length()-1] != '/') linkUri += "/";
        linkUri += name;
        
        html += "<div class='item'>";
        if (isDir) {
            html += "<a href='" + linkUri + "' class='folder'>📁 " + name + "/</a>";
        } else {
            html += "<a href='" + linkUri + "' class='file'>📄 " + name + "</a>";
            html += "<button class='delete-btn' onclick=\"deleteFile('" + linkUri + "')\">Delete</button>";
            html += "<script>";
            html += "function deleteFile(path) {";
            html += "  if(!confirm('Delete this file?')) return;";
            html += "  fetch(path, {method: 'DELETE'})";
            html += "    .then(r => r.ok ? location.reload() : alert('Delete failed'))";
            html += "    .catch(() => alert('Delete failed'));";
            html += "}";
            html += "</script>";
        }
        html += "</div>";
    }
    
    html += "<a href='/' class='back-link'>← Back to Home</a>";
    html += "<div class='footer'>Webserv/1.0</div>";
    html += "</div></body></html>";
    
    Logger::debug("Generated directory listing for: " + path);
    return html;
}

std::vector<std::string> FileServer::getDirectoryEntries(const std::string& dirpath) {
    std::vector<std::string> entries;
    
    DIR* dir = opendir(dirpath.c_str());
    if (!dir) {
        return entries;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            entries.push_back(name);
        }
    }
    
    closedir(dir);
    return entries;
}

bool FileServer::isPathTraversalAttempt(const std::string& path) {
    return path.find("../") != std::string::npos || path.find("/..") != std::string::npos;
}

std::string FileServer::sanitizePath(const std::string& path) {
    // Basic path sanitization - remove double slashes, etc.
    std::string clean = path;
    
    // Replace double slashes with single slash
    size_t pos = 0;
    while ((pos = clean.find("//", pos)) != std::string::npos) {
        clean.replace(pos, 2, "/");
    }
    
    return clean;
}

HTTPResponse FileServer::loadErrorPage(int statusCode, const ServerConfig& config) {
    std::string errorPagePath = config.getErrorPage(statusCode);
    
    if (!errorPagePath.empty() && pathExists(errorPagePath)) {
        std::string content = Utils::readFile(errorPagePath);
        if (!content.empty()) {
            HTTPResponse response(statusCode);
            response.setBody(content);
            return response;
        }
    }
    
    // Fallback to default error page
    return createErrorResponse(statusCode);
}

std::string FileServer::getDefaultErrorPage(int statusCode, const std::string& message) {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html><head>\n";
    html << "<title>" << statusCode << " " << message << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; text-align: center; margin: 100px; }\n";
    html << "h1 { color: #d32f2f; }\n";
    html << "p { color: #666; }\n";
    html << "</style>\n";
    html << "</head><body>\n";
    html << "<h1>" << statusCode << " " << message << "</h1>\n";
    html << "<p>The requested resource could not be found or accessed.</p>\n";
    html << "<hr>\n";
    html << "<p><em>Webserv/1.0</em></p>\n";
    html << "</body></html>\n";
    
    return html.str();
}
