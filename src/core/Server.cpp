#include "Server.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include "HTTPParser.hpp"
#include "FileServer.hpp"
#include "HTTPResponse.hpp"
#include "CGIHandler.hpp"


Server::Server() : _epoll_manager(0), _config(0), _running(false) {
}

Server::~Server() {
    stop();
}

bool Server::init(Config* config) {
    if (!config || !config->isValid()) {
        Logger::error("Invalid configuration");
        return false;
    }
    
    _config = config;
    
    if (_epoll_manager.failed) {
        Logger::error("Failed to initialize epoll");
        return false;
    }
    
    // Setup listen sockets for each server configuration
    const std::vector<ServerConfig>& servers = _config->getServers();
    for (size_t i = 0; i < servers.size(); ++i) {
        if (!setupListenSocket(servers[i])) {
            Logger::error("Failed to setup listen socket for port " + Utils::intToString(servers[i].getPort()));
            return false;
        }
    }
    
    Logger::info("Server initialized successfully");
    return true;
}

bool Server::setupListenSocket(const ServerConfig& serverConfig) {
    int listen_fd = createSocket(serverConfig.getHost(), serverConfig.getPort());
    if (listen_fd == -1) {
        return false;
    }
    
    if (!makeNonBlocking(listen_fd)) {
        close(listen_fd);
        return false;
    }
    
    if (!_epoll_manager.bindToFd(listen_fd, EVENT_READ, (EpollManager::callback_t)handleClientRead)) {
        close(listen_fd);
        return false;
    }
    
    _listen_fds.push_back(listen_fd);
    Logger::info("Listening on " + serverConfig.getHost() + ":" + Utils::intToString(serverConfig.getPort()));
    return true;
}

int Server::createSocket(const std::string& host, int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        Logger::error("Failed to create socket");
        return -1;
    }
    
    // Enable address reuse
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        Logger::warning("Failed to set SO_REUSEADDR");
    }
    
    // Setup address structure
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_aton(host.c_str(), &addr.sin_addr) == 0) {
        Logger::error("Invalid host address: " + host);
        close(listen_fd);
        return -1;
    }
    
    // Bind socket
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        Logger::error("Failed to bind socket to " + host + ":" + Utils::intToString(port));
        close(listen_fd);
        return -1;
    }
    
    // Start listening
    if (listen(listen_fd, LISTEN_BACKLOG) == -1) {
        Logger::error("Failed to listen on socket");
        close(listen_fd);
        return -1;
    }
    
    return listen_fd;
}

bool Server::makeNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        Logger::error("Failed to get socket flags");
        return false;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        Logger::error("Failed to set non-blocking mode");
        return false;
    }
    
    return true;
}

bool Server::start() {
    if (_listen_fds.empty()) {
        Logger::error("No listen sockets configured");
        return false;
    }
    
    _running = true;
    Logger::info("Server started successfully");
    return true;
}

void Server::stop() {
    _running = false;
    
    // Close all client connections
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        _epoll_manager.unbindFd(it->first, -1);
    }
    _clients.clear();
    
    // Close listen sockets
    for (size_t i = 0; i < _listen_fds.size(); ++i) {
        _epoll_manager.unbindFd(_listen_fds[i], -1);
        close(_listen_fds[i]);
    }
    _listen_fds.clear();
    
    Logger::info("Server stopped");
}

void Server::run() {
    Logger::info("Server running... Press Ctrl+C to stop");
    
    while (_running) {


        if (!_epoll_manager.watchForEvents(this)) {
            Logger::error("Epoll wait failed");
            break;
        }

        // Clean up timed out clients
        std::vector<int> timed_out_clients;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second.isTimedOut()) {
                timed_out_clients.push_back(it->first);
            }
        }
        
        for (size_t i = 0; i < timed_out_clients.size(); ++i) {
            Logger::debug("Client " + Utils::intToString(timed_out_clients[i]) + " timed out");
            removeClient(timed_out_clients[i]);
        }
    }
}

// void Server::handleEvents() {
//     const std::vector<Event>& events = _epoll_manager.getReadyEvents();
    
//     for (size_t i = 0; i < events.size(); ++i) {
//         const Event& event = events[i];
        
//         if (isListenSocket(event.fd)) {
//             handleNewConnection(event.fd);
//         } else {
//             switch (event.type) {
//                 case EVENT_READ:
//                     handleClientRead(event.fd);
//                     break;
//                 case EVENT_WRITE:
//                     handleClientWrite(event.fd);
//                     break;
//                 case EVENT_ERROR:
//                     handleClientError(event.fd);
//                     break;
//             }
//         }
//     }
// }

void Server::handleNewConnection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::error("Failed to accept connection");
        }
        return;
    }
    
    Logger::info("New client connection: " + Utils::intToString(client_fd));
    addClient(client_fd);
}

void Server::handleClientRead(int client_fd, Server *server) {
    if (server->isListenSocket(client_fd)) {
        Logger::warning("HERE");
        server->handleNewConnection(client_fd);
        return;
    }
    std::map<int, Client>::iterator it = server->_clients.find(client_fd);
    if (it == server->_clients.end()) {
        return;
    }
    
    Client& client = it->second;
    
    // BOUCLE tant qu'il y a des donnees disponibles
    while (true) {
        ssize_t bytes_read = client.readData();
        
        if (bytes_read <= 0) {
            if (bytes_read == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                server->removeClient(client_fd);
            }
            break;  // Plus de donnees disponibles pour l'instant
        }
        
        // Traiter la requete apres chaque lecture
        server->processRequest(client);
        
        // Si la requete est complete, pas besoin de lire plus
        if (client.getRequest().isComplete()) {
            break;
        }
    }
}

void Server::handleClientWrite(int client_fd, Server *server) {
    std::map<int, Client>::iterator it = server->_clients.find(client_fd);
    if (it == server->_clients.end()) {
        return;
    }
    
    Client& client = it->second;
    ssize_t bytes_written = client.writeData();
    
    if (bytes_written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        server->removeClient(client_fd);
        return;
    }
    
    if (client.isWriteComplete()) {
        client.setState(DONE);
        server->removeClient(client_fd);
    }
}

void Server::handleClientError(int client_fd,  Server *server) {
    Logger::debug("Client error on fd " + Utils::intToString(client_fd));
    server->resetClientAfterError(client_fd);
    server->removeClient(client_fd);
}

void Server::addClient(int fd) {
    if (!makeNonBlocking(fd)) {
        close(fd);
        return;
    }
    
    if (!_epoll_manager.bindToFd(fd, EVENT_READ, (EpollManager::callback_t)handleClientRead)) {
        close(fd);
        return;
    }

    if (!_epoll_manager.bindToFd(fd, EVENT_ERROR, (EpollManager::callback_t)handleClientError)) {
        close(fd);
        return;
    }
    
    Client client(fd);
    _clients[fd] = client;
}

void Server::removeClient(int client_fd) {
    std::map<int, Client>::iterator it = _clients.find(client_fd);
    if (it != _clients.end()) {
  //it->second.getRequest().clear();
        _epoll_manager.unbindFd(client_fd, -1);
        it->second.closeFd();  // Ferme le fd
        _clients.erase(it);
        Logger::debug("Client " + Utils::intToString(client_fd) + " removed");
    }
}

void Server::processRequest(Client& client) {
    const std::string& buffer = client.getReadBuffer();
    
    // Utiliser le parser et la requete du client
    HTTPParser& parser = client.getParser();
    HTTPRequest& request = client.getRequest();
    
    // Parse avec le parser persistant (le parser garde son etat)
    if (!parser.parse(request, buffer)) {
        if (parser.hasError()) {
            // Send 400 Bad Request et reset le client
            Logger::warning("Parser error for client " + Utils::intToString(client.getFd()));
            resetClientAfterError(client.getFd());
            
            std::string response = createHttpResponse(400, "<h1>400 Bad Request</h1>");
            client.setWriteBuffer(response);
            client.setState(SENDING_RESPONSE);
            _epoll_manager.bindToFd(client.getFd(), EVENT_WRITE, (EpollManager::callback_t)handleClientWrite);
            return;
        }
        // Need more data - le parser attend plus de chunks
        Logger::debug("Parser needs more data, waiting... (client " + Utils::intToString(client.getFd()) + ")");
        return;
    }
    
    // Only process if request is COMPLETE
    if (!request.isComplete()) {
        Logger::debug("Request not complete, waiting for more data... (client " + Utils::intToString(client.getFd()) + ")");
        return;
    }
    
    // Request parsed successfully and complete
    Logger::info("Parsed complete request: " + request.methodToString() + " " + request.getURI());
    
    // Clear the read buffer now that we've parsed the complete request
    client.clearReadBuffer();
    
    // Generate appropriate response based on the request
    generateHttpResponse(client, request);
    client.setState(SENDING_RESPONSE);
    _epoll_manager.bindToFd(client.getFd(), EVENT_WRITE, (EpollManager::callback_t)handleClientWrite);
   // client.getRequest().clear();
}

void Server::generateHttpResponse(Client& client, const HTTPRequest& request) {
    ServerConfig* serverConfig = _config->getServerByPort(8080);
    if (!serverConfig) {
        std::string errorResponse = createHttpResponse(500, "<h1>500 Internal Server Error</h1>");
        client.setWriteBuffer(errorResponse);
        return;
    }
    
    // Find matching location
    const LocationConfig* location = serverConfig->findLocation(request.getURI());
    
    if (!location) {
        HTTPResponse response = FileServer::serveFile(request, *serverConfig);
        client.setWriteBuffer(response.toString());
        Logger::info("Served: " + request.methodToString() + " " + request.getURI() + " -> " + 
                    Utils::intToString(response.getStatusCode()));
        return;
    }
    
    // CHECK IF CGI IS ENABLED AND URI MATCHES CGI EXTENSION
    if (location->cgi_enabled && !location->cgi_extension.empty()) {
        std::string uri = request.getURI();
        
        // Check if URI ends with CGI extension
        if (Utils::endsWith(uri, location->cgi_extension)) {
            Logger::debug("CGI request detected: " + uri);
            
        // Build script path - remove location path from URI first
        std::string relativePath = uri.substr(location->path.length());
        std::string scriptPath = location->root + relativePath;

        Logger::debug("Script path: " + scriptPath);
            
            // Check if script exists
            if (!Utils::fileExists(scriptPath)) {
                HTTPResponse response(404);
                response.setBody("<h1>404 - CGI Script Not Found</h1>");
                client.setWriteBuffer(response.toString());
                Logger::warning("CGI script not found: " + scriptPath);
                return;
            }
            
            // Execute CGI
            CGIHandler cgiHandler(request, *location, scriptPath);
            HTTPResponse response;
            
            if (cgiHandler.execute(response)) {
                client.setWriteBuffer(response.toString());
                Logger::info("CGI executed: " + request.methodToString() + " " + request.getURI() + " -> " + 
                            Utils::intToString(response.getStatusCode()));
            } else {
                response.setStatusCode(500);
                response.setBody("<h1>500 - CGI Execution Failed</h1>");
                client.setWriteBuffer(response.toString());
                Logger::error("CGI execution failed: " + scriptPath);
            }
            return;
        }
    }
    
    // Normal file serving
    HTTPResponse response = FileServer::serveFile(request, *serverConfig);
    client.setWriteBuffer(response.toString());
    
    Logger::info("Served: " + request.methodToString() + " " + request.getURI() + " -> " + 
                Utils::intToString(response.getStatusCode()));
}

void Server::generateResponse(Client& client, const std::string& request) {
    // Simple response for now
    std::string content = "<html><body><h1>Hello from Webserv!</h1><p>Request received:</p><pre>" + request + "</pre></body></html>";
    std::string response = createHttpResponse(200, content);
    client.setWriteBuffer(response);
    
    Logger::debug("Generated response for client " + Utils::intToString(client.getFd()));
}

std::string Server::createHttpResponse(int statusCode, const std::string& content, const std::string& contentType) {
    std::string response;
    
    response += "HTTP/1.1 " + Utils::intToString(statusCode) + " " + getStatusMessage(statusCode) + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + Utils::intToString(content.length()) + "\r\n";
    response += "Date: " + getCurrentHttpDate() + "\r\n";
    response += "Server: Webserv/1.0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += content;
    
    return response;
}

std::string Server::getStatusMessage(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

std::string Server::getCurrentHttpDate() {
    return Utils::formatHttpDate(time(0));
}

bool Server::isListenSocket(int fd) {
    return std::find(_listen_fds.begin(), _listen_fds.end(), fd) != _listen_fds.end();
}

void Server::resetClientAfterError(int client_fd) {
    std::map<int, Client>::iterator it = _clients.find(client_fd);
    if (it != _clients.end()) {
        it->second.clearReadBuffer();
        it->second.getParser().reset();
        it->second.getRequest() = HTTPRequest();
        Logger::debug("Client " + Utils::intToString(client_fd) + " reset after error");
    }
}

