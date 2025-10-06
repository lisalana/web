#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "HTTPRequest.hpp"
#include <string>

enum ParserState {
    PARSING_REQUEST_LINE,
    PARSING_HEADERS,
    PARSING_BODY,
    PARSING_COMPLETE,
    PARSING_ERROR
};

class HTTPParser {
private:
    ParserState _state;
    std::string _buffer;
    HTTPRequest* _request;
    size_t _bytes_parsed;
    size_t _body_bytes_received;

public:
    HTTPParser();
    ~HTTPParser();
    
    // Main parsing function
    bool parse(HTTPRequest& request, const std::string& data);
    bool parse(HTTPRequest& request, const char* data, size_t length);
    
    // State management
    ParserState getState() const;
    void reset();
    
    // Utils
    bool isComplete() const;
    bool hasError() const;
    size_t getBytesParsed() const;

private:
    bool parseRequestLine(const std::string& line);
    bool parseHeader(const std::string& line);
    bool parseBody();
    bool parseChunkedBody();
    
    HTTPMethod stringToMethod(const std::string& method);
    HTTPVersion stringToVersion(const std::string& version);
    std::string getNextLine();
    bool hasCompleteLine() const;
    void setState(ParserState state);
    
    bool isValidMethod(const std::string& method);
    bool isValidVersion(const std::string& version);
    bool isValidURI(const std::string& uri);
    bool isValidHeaderName(const std::string& name);
};

#endif