#include "CGIHandler.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>


CGIHandler::CGIHandler(const HTTPRequest& request, const LocationConfig& location, const std::string& scriptPath)
    : _request(&request), _location(&location), _scriptPath(scriptPath) {
}

CGIHandler::~CGIHandler() {
}

bool CGIHandler::execute(HTTPResponse& response) {
    Logger::debug("Executing CGI: " + _scriptPath);
    
    // Setup environment variables
    setupEnvironment();
    
    // Execute CGI
    std::string output;
    if (!executeCGI(output)) {
        Logger::error("Failed to execute CGI");
        response.setStatusCode(500);
        return false;
    }
    
    // Parse CGI output
    if (!parseCGIOutput(output, response)) {
        Logger::error("Failed to parse CGI output");
        response.setStatusCode(500);
        return false;
    }
    
    return true;
}

void CGIHandler::setupEnvironment() {
    // Mandatory CGI variables
    _env["REQUEST_METHOD"] = _request->methodToString();
    _env["SERVER_PROTOCOL"] = _request->versionToString();
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["SCRIPT_NAME"] = getScriptFilename();
    _env["PATH_INFO"] = getPathInfo();
    _env["QUERY_STRING"] = _request->getQueryString();
    _env["CONTENT_LENGTH"] = Utils::intToString(_request->getContentLength());
    _env["CONTENT_TYPE"] = _request->getHeader("content-type");
    
    // Server variables
    _env["SERVER_SOFTWARE"] = "Webserv/1.0";
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_NAME"] = _request->getHeader("host");
    _env["REDIRECT_STATUS"] = "200";
    
    // HTTP headers as CGI variables
    const std::map<std::string, std::string>& headers = _request->getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string key = "HTTP_" + Utils::toUpperCase(it->first);
        std::replace(key.begin(), key.end(), '-', '_');
        _env[key] = it->second;
    }
    
    Logger::debug("CGI environment prepared with " + Utils::intToString(_env.size()) + " variables");
}

char** CGIHandler::createEnvArray() {
    char** env = new char*[_env.size() + 1];
    size_t i = 0;
    
    for (std::map<std::string, std::string>::const_iterator it = _env.begin();
         it != _env.end(); ++it, ++i) {
        std::string entry = it->first + "=" + it->second;
        env[i] = new char[entry.length() + 1];
        std::strcpy(env[i], entry.c_str());
    }
    env[i] = NULL;
    
    return env;
}

void CGIHandler::freeEnvArray(char** env) {
    for (size_t i = 0; env[i] != NULL; ++i) {
        delete[] env[i];
    }
    delete[] env;
}

bool CGIHandler::executeCGI(std::string& output) {
    int pipeIn[2];
    int pipeOut[2];
    
    if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
        Logger::error("Failed to create pipes");
        return false;
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        Logger::error("Fork failed");
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        return false;
    }
    
    if (pid == 0) {
        // Child process
        close(pipeIn[1]);
        close(pipeOut[0]);

        // Redirect stdin/stdout
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);

        close(pipeIn[0]);
        close(pipeOut[1]);

        // Prepare arguments
        char* argv[3];  // Programme + script + NULL
        argv[0] = const_cast<char*>(_location->cgi_path.c_str());
        argv[1] = const_cast<char*>(_scriptPath.c_str());
        argv[2] = NULL;

        // Prepare environment
        char** env = createEnvArray();

        // Execute CGI
        execve(_location->cgi_path.c_str(), argv, env);

        // If execve returns, it failed
        Logger::error("execve failed");
        exit(1);
    }   
    
    // Parent process
    close(pipeIn[0]);
    close(pipeOut[1]);
    
    // Send request body to CGI
    if (_request->getContentLength() > 0) {
        const std::string& body = _request->getBody();
        write(pipeIn[1], body.c_str(), body.length());
    }
    close(pipeIn[1]);

    // Read CGI & rend pipe non blocant

    fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);

    // Read CGI output avec timeout
    char buffer[8192];
    time_t start = time(NULL);
    int timeout_seconds = 5;
    bool timeout_occurred = false;

    while (true) {
        // Verif le timeout
        if (difftime(time(NULL), start) > timeout_seconds) {
            Logger::warning("CGI timeout, killing process");
            kill(pid, SIGKILL);
            timeout_occurred = true;
            break;
        }

        // Essaie de lire
        ssize_t bytesRead = read(pipeOut[0], buffer, sizeof(buffer));

        
        // remove errno checks and replace them
        if (bytesRead > 0) {
            output.append(buffer, bytesRead);
            Logger::debug("Read " + Utils::intToString(bytesRead) + " bytes from CGI");
        } else if (bytesRead == 0) {
            // EOF - process termine
            break;
        } else if (bytesRead == -1) {
            // Pas de donnee dispo, attends un peu
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(pipeOut[0], &read_fds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            select(pipeOut[0] + 1, &read_fds, NULL, NULL, &tv);
        } else {
            Logger::error("Read error from CGI");
            break;
        }
    }

    close(pipeOut[0]);

    if (timeout_occurred) {
        waitpid(pid, NULL, 0);
        return false;
    }

    // Wait for child
    int status;
    waitpid(pid, &status, 0);

    Logger::debug("Total CGI output: " + Utils::intToString(output.length()) + " bytes");
    Logger::debug("Child exit status: " + Utils::intToString(WEXITSTATUS(status)));
    Logger::debug("Child exit status: " + Utils::intToString(WEXITSTATUS(status)));

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        Logger::debug("CGI executed successfully, output: " + Utils::intToString(output.length()) + " bytes");
        return true;
    }
    
    Logger::error("CGI exited with error");
    return false;
}

bool CGIHandler::parseCGIOutput(const std::string& output, HTTPResponse& response) {
    // Find headers/body separator
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        if (headerEnd == std::string::npos) {
            // No headers, treat all as body
            response.setBody(output);
            response.setStatusCode(200);
            return true;
        }
        headerEnd += 2;
    } else {
        headerEnd += 4;
    }
    
    // Parse headers
    std::string headerSection = output.substr(0, headerEnd);
    std::vector<std::string> lines = Utils::split(headerSection, '\n');
    
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = Utils::trim(lines[i]);
        if (line.empty()) continue;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = Utils::trim(line.substr(0, colonPos));
            std::string value = Utils::trim(line.substr(colonPos + 1));
            
            if (Utils::toLowerCase(name) == "status") {
                int statusCode = Utils::stringToInt(value.substr(0, 3));
                response.setStatusCode(statusCode);
            } else {
                response.addHeader(name, value);
            }
        }
    }
    
    // Set body
    std::string body = output.substr(headerEnd);
    response.setBody(body);
    
    // Default status if not set
    if (response.getStatusCode() == 200 && !response.hasHeader("Status")) {
        response.setStatusCode(200);
    }
    
    return true;
}

std::string CGIHandler::getScriptFilename() const {
    return _request->getURI();
}

std::string CGIHandler::getPathInfo() const {
    return _request->getURI();
}