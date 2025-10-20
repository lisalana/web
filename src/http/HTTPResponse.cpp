#include "HTTPResponse.hpp"
#include "Utils.hpp"
#include <ctime>
#include <sstream>

#define CRLF "\r\n"

HTTPResponse::HTTPResponse() : _status_code(200), _stopserver(false){
    setDefaultHeaders();
}

HTTPResponse::HTTPResponse(int statusCode) : _status_code(statusCode), _stopserver(false) {
    _status_message = getStatusMessage(statusCode);
    setDefaultHeaders();
}

HTTPResponse::~HTTPResponse() {
}

void HTTPResponse::setStatusCode(int code) {
    _status_code = code;
    _status_message = getStatusMessage(code);
}

void HTTPResponse::setStatusMessage(const std::string& message) {
    _status_message = message;
}

void HTTPResponse::setBody(const std::string& body) {
    _body = body;
    setContentLength(_body.length());
}

void HTTPResponse::addHeader(const std::string& name, const std::string& value) {
    std::string lower_name = Utils::toLowerCase(name);
    
    // permettre plusieurs valeurs
    if (lower_name == "set-cookie") {
        // Si un Set-Cookie existe deja on l'accumule sur plusieurs lignes
        if (_headers.find(lower_name) != _headers.end()) {
            _headers[lower_name] += "\r\nSet-Cookie: " + value;
        } else {
            _headers[lower_name] = value;
        }
    } else {
        // Pour les autres headers, on remplace
        _headers[lower_name] = value;
    }
}

void HTTPResponse::setHeader(const std::string& name, const std::string& value) {
    _headers[Utils::toLowerCase(name)] = value;
}

int HTTPResponse::getStatusCode() const {
    return _status_code;
}

const std::string& HTTPResponse::getStatusMessage() const {
    return _status_message;
}

const std::string& HTTPResponse::getBody() const {
    return _body;
}

std::string HTTPResponse::getHeader(const std::string& name) const {
    std::string lower_name = Utils::toLowerCase(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lower_name);
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

std::string HTTPResponse::toString() const {
    std::string response;
    
    // Status line
    response += "HTTP/1.1 " + Utils::intToString(_status_code) + " " + _status_message + CRLF;
    
    // Headers
    response += headerToString();
    
    // Empty line before body
    response += CRLF;
    
    // Body
    response += _body;
    
    return response;
}

void HTTPResponse::setContentType(const std::string& contentType) {
    setHeader("Content-Type", contentType);
}

void HTTPResponse::setContentLength(size_t length) {
    setHeader("Content-Length", Utils::intToString(length));
}

void HTTPResponse::setConnection(const std::string& connection) {
    setHeader("Connection", connection);
}

void HTTPResponse::clear() {
    _status_code = 200;
    _status_message = "OK";
    _headers.clear();
    _body.clear();
    setDefaultHeaders();
}

bool HTTPResponse::hasHeader(const std::string& name) const {
    std::string lower_name = Utils::toLowerCase(name);
    return _headers.find(lower_name) != _headers.end();
}

void HTTPResponse::setDefaultHeaders() {
    setHeader("Server", "Webserv/1.0");
    setHeader("Date", getCurrentHttpDate());
    setHeader("Connection", "close");
    setHeader("Content-Type", "text/html; charset=UTF-8");
}

std::string HTTPResponse::headerToString() const {
    std::string headers;
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        if (it->first == "set-cookie") {
            headers += "Set-Cookie: " + it->second + CRLF;
        } else {
            headers += it->first + ": " + it->second + CRLF;
        }
    }
    return headers;
}

std::string HTTPResponse::getStatusMessage(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Request Entity Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

std::string HTTPResponse::getCurrentHttpDate() {
    return Utils::formatHttpDate(time(0));
}

std::string HTTPResponse::getContentTypeByExtension(const std::string& filename) {
    if (filename.empty()) {
        return "application/octet-stream";
    }
    
    // Find the last dot for extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string extension = Utils::toLowerCase(filename.substr(dot_pos + 1));
    
    // HTML and text files
    if (extension == "html" || extension == "htm") {
        return "text/html; charset=UTF-8";
    }
    if (extension == "css") {
        return "text/css; charset=UTF-8";
    }
    if (extension == "js") {
        return "application/javascript; charset=UTF-8";
    }
    if (extension == "txt") {
        return "text/plain; charset=UTF-8";
    }
    if (extension == "json") {
        return "application/json; charset=UTF-8";
    }
    if (extension == "xml") {
        return "application/xml; charset=UTF-8";
    }
    
    // Images
    if (extension == "png") {
        return "image/png";
    }
    if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
    }
    if (extension == "gif") {
        return "image/gif";
    }
    if (extension == "svg") {
        return "image/svg+xml";
    }
    if (extension == "ico") {
        return "image/x-icon";
    }
    
    // Other common types
    if (extension == "pdf") {
        return "application/pdf";
    }
    if (extension == "zip") {
        return "application/zip";
    }
    
    return "application/octet-stream";
}

void HTTPResponse::setShouldStopServer(bool stop) {
    _stopserver = stop;
}

bool HTTPResponse::shouldStopServer() const {
    return _stopserver;
}
