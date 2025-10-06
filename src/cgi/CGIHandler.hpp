#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"

class CGIHandler {
private:
    const HTTPRequest* _request;
    const LocationConfig* _location;
    std::string _scriptPath;
    std::map<std::string, std::string> _env;

public:
    CGIHandler(const HTTPRequest& request, const LocationConfig& location, const std::string& scriptPath);
    ~CGIHandler();
    
    // Execute CGI and return response
    bool execute(HTTPResponse& response);
    
private:
    // Setup environment variables
    void setupEnvironment();
    char** createEnvArray();
    void freeEnvArray(char** env);
    
    // Execute the CGI script
    bool executeCGI(std::string& output);
    
    // Parse CGI output
    bool parseCGIOutput(const std::string& output, HTTPResponse& response);
    
    // Utils
    std::string getScriptFilename() const;
    std::string getPathInfo() const;
};

#endif