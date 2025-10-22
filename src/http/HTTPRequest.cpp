#include "HTTPRequest.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

HTTPRequest::HTTPRequest() {
    clear();
}

HTTPRequest::~HTTPRequest() {
}

void HTTPRequest::clear() {
    _method = METHOD_UNKNOWN;
    _uri = "";
    _query_string = "";
    _version = HTTP_UNKNOWN;
    _headers.clear();
    _body = "";
    _is_complete = false;
    _is_valid = false;
    _content_length = 0;
    _is_chunked = false;
    _chunked_complete = false;
}

HTTPMethod HTTPRequest::getMethod() const {
    return _method;
}

const std::string& HTTPRequest::getURI() const {
    return _uri;
}

const std::string& HTTPRequest::getQueryString() const {
    return _query_string;
}

HTTPVersion HTTPRequest::getVersion() const {
    return _version;
}

const std::string& HTTPRequest::getBody() const {
    return _body;
}

const std::map<std::string, std::string>& HTTPRequest::getHeaders() const {
    return _headers;
}

size_t HTTPRequest::getContentLength() const {
    return _content_length;
}

std::string HTTPRequest::getHeader(const std::string& name) const {
    std::string lower_name = toLowerCase(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lower_name);
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

bool HTTPRequest::hasHeader(const std::string& name) const {
    std::string lower_name = toLowerCase(name);
    return _headers.find(lower_name) != _headers.end();
}

bool HTTPRequest::isComplete() const {
    return _is_complete;
}

bool HTTPRequest::isValid() const {
    return _is_valid;
}

int HTTPRequest::getPort() const {
    std::string host = getHeader("host");

    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        return Utils::stringToInt(host.substr(colon_pos + 1));
    }
    return 8080;
}

bool HTTPRequest::isChunked() const {
    return _is_chunked;
}

void HTTPRequest::setMethod(HTTPMethod method) {
    _method = method;
}

void HTTPRequest::setURI(const std::string& uri) {
    // Parse URI and query string
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        _uri = uri.substr(0, query_pos);
        _query_string = uri.substr(query_pos + 1);
    } else {
        _uri = uri;
        _query_string = "";
    }
}

void HTTPRequest::setQueryString(const std::string& query) {
    _query_string = query;
}

void HTTPRequest::setVersion(HTTPVersion version) {
    _version = version;
}

void HTTPRequest::setBody(const std::string& body) {
    _body = body;
}

void HTTPRequest::addHeader(const std::string& name, const std::string& value) {
    std::string lower_name = toLowerCase(name);
    _headers[lower_name] = Utils::trim(value);
    
    // Update content length and chunked status when relevant headers are added
    if (lower_name == "content-length") {
        parseContentLength();
    } else if (lower_name == "transfer-encoding") {
        checkIfChunked();
    }
}

void HTTPRequest::setComplete(bool complete) {
    _is_complete = complete;
}

void HTTPRequest::setValid(bool valid) {
    _is_valid = valid;
}

std::string HTTPRequest::methodToString() const {
    switch (_method) {
        case METHOD_GET:    return "GET";
        case METHOD_POST:   return "POST";
        case METHOD_DELETE: return "DELETE";
        default:            return "UNKNOWN";
    }
}

std::string HTTPRequest::versionToString() const {
    switch (_version) {
        case HTTP_1_0: return "HTTP/1.0";
        case HTTP_1_1: return "HTTP/1.1";
        default:       return "HTTP/UNKNOWN";
    }
}

void HTTPRequest::parseContentLength() {
    std::string length_str = getHeader("content-length");
    if (!length_str.empty()) {
        std::stringstream ss(length_str);
        ss >> _content_length;
        if (ss.fail()) {
            _content_length = 0;
        }
    }
}

void HTTPRequest::checkIfChunked() {
    std::string transfer_encoding = getHeader("transfer-encoding");
    _is_chunked = (toLowerCase(transfer_encoding) == "chunked");
}

std::string HTTPRequest::toLowerCase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string& HTTPRequest::getBodyRef() {
    return _body;
}

bool HTTPRequest::isChunkedComplete() const {
    return _chunked_complete;
}

void HTTPRequest::setChunkedComplete(bool complete) {
    _chunked_complete = complete;
}