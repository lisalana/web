#include "HTTPParser.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include <sstream>
#include <cstdlib>

HTTPParser::HTTPParser() {
    reset();
}

HTTPParser::~HTTPParser() {
}

void HTTPParser::reset() {
    _state = PARSING_REQUEST_LINE;
    _buffer.clear();
    _request = 0;
    _bytes_parsed = 0;
    _body_bytes_received = 0;
}

bool HTTPParser::parse(HTTPRequest& request, const std::string& data) {
    return parse(request, data.c_str(), data.length());
}

bool HTTPParser::parse(HTTPRequest& request, const char* data, size_t length) {
    if (!data || length == 0) {
        return false;
    }
    
    if (_request == 0 || _state == PARSING_REQUEST_LINE) {
        _request = &request;
    }
    
    _buffer.append(data, length);
    _bytes_parsed += length;
    
    // Debug: afficher l'état actuel
    Logger::debug("Parser state: " + Utils::intToString(_state) + 
                  ", buffer size: " + Utils::intToString(_buffer.length()));
    
    while (_state != PARSING_COMPLETE && _state != PARSING_ERROR) {
        switch (_state) {
            case PARSING_REQUEST_LINE:
                if (hasCompleteLine()) {
                    std::string line = getNextLine();
                    if (!parseRequestLine(line)) {
                        setState(PARSING_ERROR);
                        return false;
                    }
                    setState(PARSING_HEADERS);
                } else {
                    Logger::debug("Need more data for request line");
                    return true;
                }
                break;
                
            case PARSING_HEADERS:
                while (hasCompleteLine()) {
                    std::string line = getNextLine();
                    
                    if (line.empty()) {
                        Logger::debug("Headers parsing complete, switching to body");
                        setState(PARSING_BODY);
                        break;
                    }
                    
                    if (!parseHeader(line)) {
                        setState(PARSING_ERROR);
                        return false;
                    }
                }
                
                if (_state == PARSING_HEADERS) {
                    Logger::debug("Need more data for headers");
                    return true;
                }
                break;
                
            case PARSING_BODY:
                if (!parseBody()) {
                    Logger::debug("Need more data for body (have " + 
                                Utils::intToString(_buffer.length()) + " bytes, need " + 
                                Utils::intToString(_request->getContentLength()) + " bytes)");
                    return true;
                }
                setState(PARSING_COMPLETE);
                break;
                
            default:
                setState(PARSING_ERROR);
                return false;
        }
    }
    
    if (_state == PARSING_COMPLETE) {
        _request->setComplete(true);
        _request->setValid(true);
        Logger::debug("Request parsing COMPLETE");
        return true;
    }
    
    return false;
}

bool HTTPParser::parseRequestLine(const std::string& line) {
    std::vector<std::string> parts = Utils::split(line, ' ');
    
    if (parts.size() != 3) {
        Logger::debug("Invalid request line format: " + line);
        return false;
    }
    
    HTTPMethod method = stringToMethod(parts[0]);
    if (method == METHOD_UNKNOWN) {
        Logger::debug("Unknown HTTP method: " + parts[0]);
        return false;
    }
    
    if (!isValidURI(parts[1])) {
        Logger::debug("Invalid URI: " + parts[1]);
        return false;
    }
    
    HTTPVersion version = stringToVersion(parts[2]);
    if (version == HTTP_UNKNOWN) {
        Logger::debug("Unknown HTTP version: " + parts[2]);
        return false;
    }
    
    _request->setMethod(method);
    _request->setURI(parts[1]);
    _request->setVersion(version);
    
    Logger::debug("Parsed request line: " + parts[0] + " " + parts[1] + " " + parts[2]);
    return true;
}

bool HTTPParser::parseHeader(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        Logger::debug("Invalid header format: " + line);
        return false;
    }
    
    std::string name = Utils::trim(line.substr(0, colon_pos));
    std::string value = Utils::trim(line.substr(colon_pos + 1));
    
    if (!isValidHeaderName(name)) {
        Logger::debug("Invalid header name: " + name);
        return false;
    }
    
    _request->addHeader(name, value);
    Logger::debug("Parsed header: " + name + " = " + value);
    return true;
}

bool HTTPParser::parseBody() {
    Logger::debug("parseBody called, expected length: " + Utils::intToString(_request->getContentLength()));
    Logger::debug("Current buffer length: " + Utils::intToString(_buffer.length()));
    
    // For GET requests, no body expected
    if (_request->getMethod() == METHOD_GET || _request->getMethod() == METHOD_DELETE) {
        return true;
    }
    
    // Check if we have Content-Length
    size_t expected_length = _request->getContentLength();
    
    if (expected_length == 0 && !_request->isChunked()) {
        // No body expected
        return true;
    }
    
    if (_request->isChunked()) {
        return parseChunkedBody();
    }
    
    // Check if we have enough data for the body
    if (_buffer.length() >= expected_length) {
        std::string body = _buffer.substr(0, expected_length);
        _buffer = _buffer.substr(expected_length);
        _request->setBody(body);
        Logger::debug("Parsed body: " + Utils::intToString(body.length()) + " bytes");
        return true;
    }
    return false; // need more data
}

HTTPMethod HTTPParser::stringToMethod(const std::string& method) {
    if (method == "GET") return METHOD_GET;
    if (method == "POST") return METHOD_POST;
    if (method == "DELETE") return METHOD_DELETE;
    if (method == "PUT") return METHOD_PUT;
    return METHOD_UNKNOWN;
}

HTTPVersion HTTPParser::stringToVersion(const std::string& version) {
    if (version == "HTTP/1.0") return HTTP_1_0;
    if (version == "HTTP/1.1") return HTTP_1_1;
    return HTTP_UNKNOWN;
}

std::string HTTPParser::getNextLine() {
    size_t line_end = _buffer.find("\r\n");
    if (line_end == std::string::npos) {
        return "";
    }
    
    std::string line = _buffer.substr(0, line_end);
    _buffer = _buffer.substr(line_end + 2);
    return line;
}

bool HTTPParser::hasCompleteLine() const {
    return _buffer.find("\r\n") != std::string::npos;
}

ParserState HTTPParser::getState() const {
    return _state;
}

void HTTPParser::setState(ParserState state) {
    _state = state;
}

bool HTTPParser::isComplete() const {
    return _state == PARSING_COMPLETE;
}

bool HTTPParser::hasError() const {
    return _state == PARSING_ERROR;
}

size_t HTTPParser::getBytesParsed() const {
    return _bytes_parsed;
}

bool HTTPParser::isValidMethod(const std::string& method) {
    return method == "GET" || method == "POST" || method == "DELETE";
}

bool HTTPParser::isValidVersion(const std::string& version) {
    return version == "HTTP/1.0" || version == "HTTP/1.1";
}

bool HTTPParser::isValidURI(const std::string& uri) {
    if (uri.empty() || uri[0] != '/') {
        return false;
    }
    
    // Basic URI validation
    for (size_t i = 0; i < uri.length(); ++i) {
        char c = uri[i];
        if (c < 32 || c > 126) 
        {
            return false;
        }
    }
    
    return true;
}

bool HTTPParser::isValidHeaderName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    
    for (size_t i = 0; i < name.length(); ++i) {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || c == '-' || c == '_')) 
        {
            return false;
        }
    }
    
    return true;
}


bool HTTPParser::parseChunkedBody() {
    std::string& body = _request->getBodyRef();
    
    while (true) {
        // Chercher la fin de la ligne de taille du chunk
        size_t crlf_pos = _buffer.find("\r\n");
        if (crlf_pos == std::string::npos) {
            return false; // Besoin de plus de données
        }
        
        // Parser la taille du chunk (en hexadécimal)
        std::string size_line = _buffer.substr(0, crlf_pos);
        
        // Ignorer les extensions de chunk (après ';')
        size_t semicolon = size_line.find(';');
        if (semicolon != std::string::npos) {
            size_line = size_line.substr(0, semicolon);
        }
        
        // Trim whitespace
        size_line = Utils::trim(size_line);
        
        // Convertir hexa -> decimal
        size_t chunk_size;
        char* endptr;
        chunk_size = strtoul(size_line.c_str(), &endptr, 16);  // PAS de std::

        if (*endptr != '\0') {
            Logger::error("Invalid chunk size: '" + size_line + "'");
            return false;
        }
        
        Logger::debug("Chunk size: " + Utils::intToString(chunk_size) + " (0x" + size_line + ")");
        
        // Chunk de taille 0 = fin
        if (chunk_size == 0) {
            // Consommer "0\r\n\r\n" (dernier chunk + ligne vide finale)
            size_t trailer_end = _buffer.find("\r\n\r\n", crlf_pos);
            if (trailer_end == std::string::npos) {
                return false; // Besoin de la ligne vide finale
            }
            _buffer = _buffer.substr(trailer_end + 4);
            _request->setChunkedComplete(true);
            Logger::info("Chunked body complete: " + Utils::intToString(body.length()) + " bytes");
            return true;
        }
        
        // Vérifier qu'on a assez de données pour ce chunk
        // Format: "SIZE\r\nDATA\r\n"
        size_t needed = crlf_pos + 2 + chunk_size + 2;
        if (_buffer.length() < needed) {
            Logger::debug("Not enough data for chunk. Need: " + Utils::intToString(needed) + 
                         ", have: " + Utils::intToString(_buffer.length()));
            return false; // Besoin de plus de données
        }
        
        // Extraire les données du chunk
        std::string chunk_data = _buffer.substr(crlf_pos + 2, chunk_size);
        body += chunk_data;
        
        // Consommer ce chunk du buffer
        _buffer = _buffer.substr(needed);
        
        Logger::debug("Accumulated body: " + Utils::intToString(body.length()) + " bytes");
    }
}
