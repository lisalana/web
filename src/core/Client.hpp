#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sys/socket.h>
#include <ctime>
#include "HTTPParser.hpp"
#include "HTTPRequest.hpp"

static const int CLIENT_TIMEOUT = 120;  // 2 minutes timeout

enum ClientState {
    READING_REQUEST,
    PROCESSING_REQUEST,
    SENDING_RESPONSE,
    DONE
};

class Client {
private:
    int _fd;
    ClientState _state;
    std::string _read_buffer;
    std::string _write_buffer;
    size_t  _bytes_sent;
    size_t _write_offset;
    time_t _last_activity;
    HTTPParser _parser;      // Parser pour ce client
    HTTPRequest _request;    // Requete en cours de construction
    
    static const size_t BUFFER_SIZE = 8192;

public:
    Client();
    Client(int fd);
    ~Client();
    void closeFd();
    
    Client(const Client& other);
    Client& operator=(const Client& other);
    
    int getFd() const;
    ClientState getState() const;
    const std::string& getReadBuffer() const;
    const std::string& getWriteBuffer() const;
    size_t getWriteOffset() const;
    time_t getLastActivity() const;
    HTTPParser& getParser();
    HTTPRequest& getRequest();
    
    void setState(ClientState state);
    void setWriteBuffer(const std::string& data);
    void updateLastActivity();
    
    // I/O operations
    ssize_t readData();
    ssize_t writeData();
    
    // Buffer management
    void clearReadBuffer();
    void clearWriteBuffer();
    void appendToReadBuffer(const std::string& data);
    
    // Utils
    bool isTimedOut() const;
    bool isWriteComplete() const;
    bool hasDataToWrite() const;

private:
    void init();
};

#endif


