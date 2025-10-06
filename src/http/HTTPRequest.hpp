#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

enum HTTPMethod {
    METHOD_GET,
    METHOD_POST,
    METHOD_DELETE,
    METHOD_PUT,
    METHOD_UNKNOWN
};

enum HTTPVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_UNKNOWN
};

class HTTPRequest {
private:
    HTTPMethod _method;
    std::string _uri;
    std::string _query_string;
    HTTPVersion _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _is_complete;
    bool _is_valid;
    size_t _content_length;
    bool _is_chunked;
    bool _chunked_complete;

public:
    HTTPRequest();
    ~HTTPRequest();
    
    // Getters
    HTTPMethod getMethod() const;
    const std::string& getURI() const;
    const std::string& getQueryString() const;
    HTTPVersion getVersion() const;
    const std::string& getBody() const;
    std::string& getBodyRef();
    const std::map<std::string, std::string>& getHeaders() const;
    size_t getContentLength() const;
    
    // Header operations
    std::string getHeader(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
    
    // Status
    bool isComplete() const;
    bool isValid() const;
    bool isChunked() const;
    bool isChunkedComplete() const;
    
    // Setters (for parser)
    void setMethod(HTTPMethod method);
    void setURI(const std::string& uri);
    void setQueryString(const std::string& query);
    void setVersion(HTTPVersion version);
    void setBody(const std::string& body);
    void addHeader(const std::string& name, const std::string& value);
    void setComplete(bool complete);
    void setValid(bool valid);
    void setChunkedComplete(bool complete);
    
    // Utils
    std::string methodToString() const;
    std::string versionToString() const;
    void clear();

private:
    void parseContentLength();
    void checkIfChunked();
    std::string toLowerCase(const std::string& str) const;
};

#endif