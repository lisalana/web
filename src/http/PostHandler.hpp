#ifndef POSTHANDLER_HPP
#define POSTHANDLER_HPP

#include <string>
#include <map>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"

struct FormField 
{
    std::string name;
    std::string value;
    std::string contentType;
    std::string filename;
    bool isFile;
    
    FormField() : isFile(false) {}
};

class PostHandler 
{
public:
    static HTTPResponse handlePost(const HTTPRequest& request, const ServerConfig& config);
    static HTTPResponse handleFileUpload(const HTTPRequest& request, const LocationConfig& location, const ServerConfig& serverConfig);
    static HTTPResponse handleFormData(const HTTPRequest& request, const LocationConfig& location);
    
private:
    // Form parsing
    static std::map<std::string, FormField> parseMultipartFormData(const std::string& body, const std::string& boundary);
    static std::map<std::string, std::string> parseUrlEncodedData(const std::string& body);
    
    // File operations
    static bool saveUploadedFile(const FormField& field, const std::string& uploadPath);
    static std::string generateUniqueFilename(const std::string& originalName, const std::string& uploadPath);
    static bool isAllowedFileType(const std::string& filename);
    static size_t getFileSize(const FormField& field);
    
    // Boundary parsing
    static std::string extractBoundary(const std::string& contentType);
    static std::vector<std::string> splitByBoundary(const std::string& body, const std::string& boundary);
    static FormField parseFormField(const std::string& fieldData);
    
    // Response generation
    static HTTPResponse createUploadSuccessResponse(const std::vector<std::string>& uploadedFiles);
    static HTTPResponse createUploadErrorResponse(const std::string& error);
    
    // Validation
    static bool isValidUploadRequest(const HTTPRequest& request, const LocationConfig& location);
    static bool checkContentLength(const HTTPRequest& request, size_t maxSize);

    //static void debugMultipartBody(const std::string& body, const std::string& boundary);

};

#endif