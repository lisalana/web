#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>
#include "Client.hpp"
#include "Epoll.hpp"
#include "Config.hpp"
#include "HTTPRequest.hpp"

class Server {
private:
    std::vector<int> _listen_fds;
    EpollManager _epoll_manager;
    std::map<int, Client> _clients;
    Config* _config;
    bool _running;
    
    static const int LISTEN_BACKLOG = 128;

    void resetClientAfterError(int client_fd);

public:
    Server();
    ~Server();

    bool init(Config* config);
    bool start();
    void stop();
    void run();
    
private:
    // Socket setup
    bool setupListenSocket(const ServerConfig& serverConfig);
    int createSocket(const std::string& host, int port);
    bool makeNonBlocking(int fd);
    
    // Event
    void handleEvents();
    static void handleNewConnection(int listen_fd, Server *server);
    static void handleClientRead(int client_fd, Server *server);
    static void handleClientWrite(int client_fd, Server *server);
    static void handleClientError(int client_fd, Server *server);
    
    // Client management
    void addClient(int fd);
    void removeClient(int client_fd);
    void processRequest(Client& client);
    void generateResponse(Client& client, const std::string& request);
    void generateHttpResponse(Client& client, const HTTPRequest& request);
    
    // HTTP response generation
    std::string createHttpResponse(int statusCode, const std::string& content, const std::string& contentType = "text/html");
    std::string getStatusMessage(int statusCode);
    std::string getCurrentHttpDate();
    
    // Utils
    bool isListenSocket(int fd);
};

#endif