#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HTTPResponse {
private:
    int _status_code;
    std::string _status_message;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _stopserver;

public:
    HTTPResponse();
    HTTPResponse(int statusCode);
    ~HTTPResponse();
    
    // Setters
    void setStatusCode(int code);
    void setStatusMessage(const std::string& message);
    void setBody(const std::string& body);
    void addHeader(const std::string& name, const std::string& value);
    void setHeader(const std::string& name, const std::string& value);
    
    void setShouldStopServer(bool stop);
    bool shouldStopServer() const;

    // Getters
    int getStatusCode() const;
    const std::string& getStatusMessage() const;
    const std::string& getBody() const;
    std::string getHeader(const std::string& name) const;
    
    // Response building
    std::string toString() const;
    void setContentType(const std::string& contentType);
    void setContentLength(size_t length);
    void setConnection(const std::string& connection);
    
    // Utility methods
    void clear();
    bool hasHeader(const std::string& name) const;
    
    // Static helper methods
    static std::string getStatusMessage(int statusCode);
    static std::string getCurrentHttpDate();
    static std::string getContentTypeByExtension(const std::string& filename);
    
private:
    void setDefaultHeaders();
    std::string headerToString() const;
};

#endif