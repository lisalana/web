#include "PostHandler.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <algorithm>
#include "String.hpp"


HTTPResponse PostHandler::handlePost(const HTTPRequest& request, const ServerConfig& config) {
    // Find matching location
    const LocationConfig* location = config.findLocation(request.getURI());
    if (!location) {
        Logger::debug("No location found for POST: " + request.getURI());
        return HTTPResponse(404);
    }
    
    // Check if POST is allowed
    if (!config.isMethodAllowed(request.getURI(), "POST")) {
        Logger::debug("POST method not allowed for: " + request.getURI());
        return HTTPResponse(405);
    }
    
    // Check content length against server limits
    if (!checkContentLength(request, config.getClientMaxBodySize())) {
        Logger::warning("Request body too large");
        return HTTPResponse(413); // Request Entity Too Large
    }
    
    // Determine content type and handle accordingly
    std::string contentType = request.getHeader("content-type");
    
    if (contentType.find("multipart/form-data") != std::string::npos) {
        return handleFileUpload(request, *location, config);
    } else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        return handleFormData(request, *location);
    } else {
        Logger::debug("Unsupported content type: " + contentType);
        return HTTPResponse(400);
    }
}

HTTPResponse PostHandler::handleFileUpload(const HTTPRequest& request, const LocationConfig& location, const ServerConfig& serverConfig) {
    Logger::debug("Starting file upload handler");
    Logger::debug("Upload path: " + location.uploadPath);
    
    if (!isValidUploadRequest(request, location)) {
        Logger::debug("Invalid upload request");
        return createUploadErrorResponse("Invalid upload request");
    }
    
    std::string contentType = request.getHeader("content-type");
    Logger::debug("Content-Type: " + contentType);
    std::string boundary = extractBoundary(contentType);
    
    if (boundary.empty()) {
        Logger::error("No boundary found in multipart request");
        return createUploadErrorResponse("No boundary in multipart data");
    }
    
    // Parse multipart form data
    std::map<std::string, FormField> fields = parseMultipartFormData(request.getBody(), boundary);
    
    std::vector<std::string> uploadedFiles;
    
    // Chercher le champ description pour le nom personnalise
    std::string customName = "";
    std::map<std::string, FormField>::iterator descIt = fields.find("description");
    if (descIt != fields.end() && !descIt->second.value.empty()) {
        customName = Utils::trim(descIt->second.value);
        Logger::debug("Custom filename from description: " + customName);
    }
    
    // Process each field
    for (std::map<std::string, FormField>::iterator it = fields.begin(); it != fields.end(); ++it) {
        const FormField& field = it->second;
        
        if (field.isFile && !field.filename.empty()) {
            // Check file size against server config
            if (getFileSize(field) > serverConfig.getClientMaxBodySize()) {
                Logger::warning("File too large: " + field.filename + " (" +
                              Utils::intToString(getFileSize(field)) + " bytes > " +
                              Utils::intToString(serverConfig.getClientMaxBodySize()) + " bytes)");
                continue;
            }
            
            // Determine le nom final du fichier
            String finalFilename = field.filename; // nom original par defaut
            
            if (!customName.empty()) {
                // Extrai l'extension du fichier original
                size_t dotPos = field.filename.find_last_of('.');
                std::string extension = "";
                if (dotPos != std::string::npos) {
                    extension = field.filename.substr(dotPos);
                }
                finalFilename = customName + extension;
                Logger::debug("Using custom filename: " + finalFilename);
            }

            finalFilename = finalFilename.replace(" ", "_");
            
            // Check file type with final filename
            if (!isAllowedFileType(finalFilename)) {
                Logger::warning("File type not allowed: " + finalFilename);
                continue;
            }
            
            // Cree une copie modifiee du field avec le nouveau nom
            FormField modifiedField = field;
            modifiedField.filename = finalFilename;
            
            // Save file with custom name
            if (saveUploadedFile(modifiedField, location.uploadPath)) {
                uploadedFiles.push_back(finalFilename);
                Logger::info("File uploaded successfully: " + finalFilename);
            } else {
                Logger::error("Failed to save file: " + finalFilename);
            }
        }
    }
    
    if (uploadedFiles.empty()) {
        return createUploadErrorResponse("No files were uploaded");
    }
    
    return createUploadSuccessResponse(uploadedFiles);
}


// void PostHandler::debugMultipartBody(const std::string& body, const std::string& boundary) {
//     Logger::debug("=== RAW BODY DEBUG ===");
//     std::string fullBoundary = "--" + boundary;
    
//     // Afficher les premiers 1000 caractères du body
//     std::string preview = body.substr(0, 1000);
//     Logger::debug("Body preview (1000 chars):");
//     Logger::debug(preview);
    
//     // Chercher toutes les occurrences du boundary
//     size_t pos = 0;
//     int count = 0;
//     while ((pos = body.find(fullBoundary, pos)) != std::string::npos) {
//         Logger::debug("Boundary found at position: " + Utils::intToString(pos));
        
//         // Afficher 200 caractères après chaque boundary
//         size_t start = pos + fullBoundary.length();
//         if (start < body.length()) {
//             size_t len = std::min((size_t)200, body.length() - start);
//             std::string after = body.substr(start, len);
//             Logger::debug("After boundary " + Utils::intToString(count) + ": " + after);
//         }
        
//         pos += fullBoundary.length();
//         count++;
//     }
//     Logger::debug("=== END RAW DEBUG ===");
// }


HTTPResponse PostHandler::handleFormData(const HTTPRequest& request, const LocationConfig& location) {
    (void)location;
    std::map<std::string, std::string> formData = parseUrlEncodedData(request.getBody());
    
    // Create response showing received form data
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>Form Data Received</title></head><body>\n";
    html << "<h1>Form Data Received</h1>\n";
    html << "<h2>Posted Data:</h2>\n<ul>\n";
    
    for (std::map<std::string, std::string>::iterator it = formData.begin(); it != formData.end(); ++it) {
        html << "<li><strong>" << it->first << ":</strong> " << it->second << "</li>\n";
    }
    
    html << "</ul>\n<p><a href=\"/\">Back to home</a></p>\n</body></html>\n";
    
    HTTPResponse response(200);
    response.setBody(html.str());
    response.setContentType("text/html; charset=UTF-8");
    
    Logger::info("Processed form data with " + Utils::intToString(formData.size()) + " fields");
    return response;
}

std::map<std::string, FormField> PostHandler::parseMultipartFormData(const std::string& body, const std::string& boundary) {
    Logger::debug("Parsing multipart data, body length: " + Utils::intToString(body.length()));
    Logger::debug("Using boundary: " + boundary);

    //debugMultipartBody(body, boundary);

    std::map<std::string, FormField> fields;
    std::vector<std::string> parts = splitByBoundary(body, boundary);
    Logger::debug("Found " + Utils::intToString(parts.size()) + " parts");
        
    for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i].empty()) continue;
        
        Logger::debug("=== PROCESSING PART " + Utils::intToString(i) + " ===");
        Logger::debug("Part " + Utils::intToString(i) + " preview: " + parts[i].substr(0, 300));
        
        FormField field = parseFormField(parts[i]);
        if (!field.name.empty()) {
            Logger::debug("=== FIELD FOUND ===");
            Logger::debug("Name: '" + field.name + "'");
            Logger::debug("Value: '" + field.value.substr(0, 50) + "'");
            Logger::debug("Filename: '" + field.filename + "'");
            Logger::debug("==================");
            
            fields[field.name] = field;
        } else {
            Logger::debug("=== FIELD PARSING FAILED ===");
            Logger::debug("Could not parse field from part " + Utils::intToString(i));
        }
    }
    return fields;
}


std::map<std::string, std::string> PostHandler::parseUrlEncodedData(const std::string& body) {
    std::map<std::string, std::string> data;
    
    std::vector<std::string> pairs = Utils::split(body, '&');
    for (size_t i = 0; i < pairs.size(); ++i) {
        size_t eq_pos = pairs[i].find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pairs[i].substr(0, eq_pos);
            std::string value = pairs[i].substr(eq_pos + 1);
            
            std::replace(value.begin(), value.end(), '+', ' ');
            
            data[key] = value;
        }
    }
    
    return data;
}

bool PostHandler::saveUploadedFile(const FormField& field, const std::string& uploadPath) {
    // Create upload directory if it doesn't exist
    struct stat st;
    if (stat(uploadPath.c_str(), &st) != 0) {
        if (mkdir(uploadPath.c_str(), 0755) != 0) {
            Logger::error("Failed to create upload directory: " + uploadPath);
            return false;
        }
    }
    
    std::string filename = generateUniqueFilename(field.filename, uploadPath);
    std::string fullPath = uploadPath + "/" + filename;
    
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to open file for writing: " + fullPath);
        return false;
    }
    
    file.write(field.value.c_str(), field.value.length());
    file.close();
    
    Logger::debug("Saved uploaded file: " + fullPath + " (" + Utils::intToString(field.value.length()) + " bytes)");
    return true;
}

std::string PostHandler::generateUniqueFilename(const std::string& originalName, const std::string& uploadPath) {
    std::string filename = originalName;
    std::string fullPath = uploadPath + "/" + filename;
    
    // If file exists, add a number suffix
    int counter = 1;
    while (Utils::fileExists(fullPath)) {
        size_t dot_pos = originalName.find_last_of('.');
        if (dot_pos != std::string::npos) {
            filename = originalName.substr(0, dot_pos) + "_" + Utils::intToString(counter) + 
                      originalName.substr(dot_pos);
        } else {
            filename = originalName + "_" + Utils::intToString(counter);
        }
        fullPath = uploadPath + "/" + filename;
        counter++;
    }
    
    return filename;
}

bool PostHandler::isAllowedFileType(const std::string& filename) {
    // Basic file type validation - could be more comprehensive
    std::vector<std::string> allowed_extensions;
    allowed_extensions.push_back(".txt");
    allowed_extensions.push_back(".html");
    allowed_extensions.push_back(".css");
    allowed_extensions.push_back(".js");
    allowed_extensions.push_back(".png");
    allowed_extensions.push_back(".jpg");
    allowed_extensions.push_back(".jpeg");
    allowed_extensions.push_back(".gif");
    allowed_extensions.push_back(".pdf");
    allowed_extensions.push_back(".ico");
    
    std::string lower_filename = Utils::toLowerCase(filename);
    for (size_t i = 0; i < allowed_extensions.size(); ++i) {
        if (Utils::endsWith(lower_filename, allowed_extensions[i])) {
            return true;
        }
    }
    
    return false;
}

size_t PostHandler::getFileSize(const FormField& field) {
    return field.value.length();
}

std::string PostHandler::extractBoundary(const std::string& contentType) {
    size_t boundary_pos = contentType.find("boundary=");
    if (boundary_pos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(boundary_pos + 9);
    
    // Remove quotes if present
    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1, boundary.length() - 2);
    }
    
    return boundary;
}

std::vector<std::string> PostHandler::splitByBoundary(const std::string& body, const std::string& boundary) {
    std::vector<std::string> parts;
    std::string fullBoundary = "--" + boundary;
    
    Logger::debug("Splitting with boundary: " + fullBoundary);
    
    // Trouver toutes les positions des boundaries
    std::vector<size_t> boundaryPositions;
    size_t pos = 0;
    while ((pos = body.find(fullBoundary, pos)) != std::string::npos) {
        boundaryPositions.push_back(pos);
        pos += fullBoundary.length();
    }
    
    Logger::debug("Found " + Utils::intToString(boundaryPositions.size()) + " boundaries");
    
    // Extraire les parties entre les boundaries
    for (size_t i = 0; i < boundaryPositions.size() - 1; ++i) {
        size_t start = boundaryPositions[i] + fullBoundary.length();
        size_t end = boundaryPositions[i + 1];
        
        // Ignorer les CRLF après le boundary
        while (start < end && (body[start] == '\r' || body[start] == '\n')) {
            start++;
        }
        
        // Ignorer les CRLF avant le prochain boundary
        while (end > start && (body[end - 1] == '\r' || body[end - 1] == '\n')) {
            end--;
        }
        
        if (start < end) {
            std::string part = body.substr(start, end - start);
            if (!part.empty()) {
                Logger::debug("Part " + Utils::intToString(i) + " length: " + Utils::intToString(part.length()));
                parts.push_back(part);
            }
        }
    }
    
    return parts;
}


FormField PostHandler::parseFormField(const std::string& fieldData) {

    Logger::debug("=== PARSING FIELD ===");
    Logger::debug("Part content (first 200 chars): " + fieldData.substr(0, 200));
    
    FormField field;
    
    // Find the double CRLF that separates headers from data
    size_t header_end = fieldData.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return field;
    }
    
    std::string headers = fieldData.substr(0, header_end);
    std::string data = fieldData.substr(header_end + 4);
    
    // Parse Content-Disposition header
    size_t disp_pos = headers.find("Content-Disposition:");
    if (disp_pos != std::string::npos) {
        std::string disp_line = headers.substr(disp_pos);
        size_t line_end = disp_line.find("\r\n");
        if (line_end != std::string::npos) {
            disp_line = disp_line.substr(0, line_end);
        }
        
        // Extract name
        size_t name_pos = disp_line.find("name=\"");
        if (name_pos != std::string::npos) {
            name_pos += 6;
            size_t name_end = disp_line.find("\"", name_pos);
            if (name_end != std::string::npos) {
                field.name = disp_line.substr(name_pos, name_end - name_pos);
            }
        }
        
        // Extract filename if present
        size_t filename_pos = disp_line.find("filename=\"");
        if (filename_pos != std::string::npos) {
            filename_pos += 10;
            size_t filename_end = disp_line.find("\"", filename_pos);
            if (filename_end != std::string::npos) {
                field.filename = disp_line.substr(filename_pos, filename_end - filename_pos);
                field.isFile = !field.filename.empty();
            }
        }
    }
    
    // Parse Content-Type if present
    size_t type_pos = headers.find("Content-Type:");
    if (type_pos != std::string::npos) {
        std::string type_line = headers.substr(type_pos + 13);
        size_t line_end = type_line.find("\r\n");
        if (line_end != std::string::npos) {
            type_line = type_line.substr(0, line_end);
        }
        field.contentType = Utils::trim(type_line);
    }
    
    field.value = data;

    Logger::debug("Parsed field name: '" + field.name + "'");
    Logger::debug("====================");
    return field;
}

HTTPResponse PostHandler::createUploadSuccessResponse(const std::vector<std::string>& uploadedFiles) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>Upload Successful</title></head><body>\n";
    html << "<h1>Files Uploaded Successfully!</h1>\n";
    html << "<h2>Uploaded Files:</h2>\n<ul>\n";
    
    for (size_t i = 0; i < uploadedFiles.size(); ++i) {
        html << "<li>" << uploadedFiles[i] << "</li>\n";
    }
    
    html << "</ul>\n<p><a href=\"/\">Back to home</a></p>\n</body></html>\n";
    
    HTTPResponse response(200);
    response.setBody(html.str());
    response.setContentType("text/html; charset=UTF-8");
    return response;
}

HTTPResponse PostHandler::createUploadErrorResponse(const std::string& error) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>Upload Error</title></head><body>\n";
    html << "<h1>Upload Failed</h1>\n";
    html << "<p>Error: " << error << "</p>\n";
    html << "<p><a href=\"/\">Back to home</a></p>\n</body></html>\n";
    
    HTTPResponse response(400);
    response.setBody(html.str());
    response.setContentType("text/html; charset=UTF-8");
    return response;
}

bool PostHandler::isValidUploadRequest(const HTTPRequest& request, const LocationConfig& location) {
    return !location.uploadPath.empty() && request.getContentLength() > 0;
}

bool PostHandler::checkContentLength(const HTTPRequest& request, size_t maxSize) {
    return request.getContentLength() <= maxSize;
}


std::string PostHandler::handleStopRequest(const HTTPRequest& request) {
    (void)request;
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n";
    
    std::string body = "<!DOCTYPE html>\n"
                      "<html>\n"
                      "<head>\n"
                      "    <title>Server Stopping</title>\n"
                      "    <style>\n"
                      "        body { font-family: Arial; text-align: center; padding: 50px; background: #f0f0f0; }\n"
                      "        h1 { color: #dc2626; }\n"
                      "    </style>\n"
                      "</head>\n"
                      "<body>\n"
                      "    <h1>🛑 Server Shutting Down</h1>\n"
                      "    <p>The server is stopping gracefully...</p>\n"
                      "    <p>Goodbye! 👋</p>\n"
                      "</body>\n"
                      "</html>";
    
    std::ostringstream oss;
    oss << body.length();
    response += "Content-Length: " + oss.str() + "\r\n\r\n";
    response += body;
    
    std::cout << "\n[INFO] Stop request received\n" << std::endl;
    
    return response;
}