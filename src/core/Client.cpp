#include "Client.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>

Client::Client() : _fd(-1) {
    init();
}

Client::Client(int fd) : _fd(fd) {
    init();
    
    // Set socket to non-blocking
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    Logger::debug("Client created with fd " + Utils::intToString(_fd));
}

Client::~Client() {
    // _read_buffer.clear();
    // _write_buffer.clear();
    // _request.clear();
}

// void Client::closeFd() {
//     if (_fd != -1) {
//         close(_fd);
//         Logger::debug("Client with fd " + Utils::intToString(_fd) + " fd closed");
//         _fd = -1;

//         _read_buffer.clear();
//         _write_buffer.clear();
//         _request.clear();
//     }
// }

void Client::closeFd() {
    if (_fd != -1) {
        close(_fd);
        Logger::debug("Client with fd " + Utils::intToString(_fd) + " fd closed");
        _fd = -1;
    }
}

// Client::Client(const Client& other) 
//     : _fd(other._fd),
//       _state(other._state),
//       _read_buffer(other._read_buffer),
//       _write_buffer(other._write_buffer),
//       _write_offset(other._write_offset),
//       _last_activity(other._last_activity),
//       _parser(other._parser),
//       _request(other._request) {
// }

Client::Client(const Client& other) {
    _fd = other._fd;
    _bytes_sent = other._bytes_sent;
    _state = other._state;
    _read_buffer = other._read_buffer;
    _write_buffer = other._write_buffer;
    _write_offset = other._write_offset;
    _last_activity = other._last_activity;
    _parser = other._parser;
    _request = other._request;
}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        if (_fd != -1) {
            close(_fd);
        }

        _read_buffer.clear();
        _write_buffer.clear();
        
        _fd = other._fd;
        _state = other._state;
        _bytes_sent = other._bytes_sent;
        _read_buffer = other._read_buffer;
        _write_buffer = other._write_buffer;
        _write_offset = other._write_offset;
        _last_activity = other._last_activity;
        _parser = other._parser;
        _request = other._request;
    }
    return *this;
}

void Client::init() {
    _state = READING_REQUEST;
    _write_offset = 0;
    _bytes_sent = 0;
    _last_activity = time(NULL);
    _parser.reset();           // Reset le parser
    _request = HTTPRequest();  // Reset la requête
    //_request.clear();
}

int Client::getFd() const {
    return _fd;
}

ClientState Client::getState() const {
    return _state;
}

const std::string& Client::getReadBuffer() const {
    return _read_buffer;
}

const std::string& Client::getWriteBuffer() const {
    return _write_buffer;
}

size_t Client::getWriteOffset() const {
    return _write_offset;
}

time_t Client::getLastActivity() const {
    return _last_activity;
}

HTTPParser& Client::getParser() {
    return _parser;
}

HTTPRequest& Client::getRequest() {
    return _request;
}

void Client::setState(ClientState state) {
    _state = state;
}

void Client::setWriteBuffer(const std::string& data) {
    _write_buffer = data;
    _write_offset = 0;
}

void Client::updateLastActivity() {
    _last_activity = time(NULL);
}

ssize_t Client::readData() {
    if (_fd == -1) return -1;
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        _read_buffer.append(buffer, bytes_read);
        updateLastActivity();
        Logger::debug("Read " + Utils::intToString(bytes_read) + " bytes from client " + Utils::intToString(_fd)
                     + " (total buffer: " + Utils::intToString(_read_buffer.size()) + " bytes)");
    } else if (bytes_read == 0) {
        Logger::debug("Client " + Utils::intToString(_fd) + " closed connection");
    } else {
        // bytes_read < 0, check errno
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::debug("Read error from client " + Utils::intToString(_fd));
        }
    }
    
    return bytes_read;
}

ssize_t Client::writeData() {
    if (_fd == -1 || _write_buffer.empty() || _write_offset >= _write_buffer.size()) {
        return 0;
    }
    
    const char* data = _write_buffer.c_str() + _write_offset;
    size_t remaining = _write_buffer.size() - _write_offset;
    
    ssize_t bytes_sent = send(_fd, data, remaining, 0);
    
    if (bytes_sent > 0) {
        _write_offset += bytes_sent;
        updateLastActivity();
        Logger::debug("Wrote " + Utils::intToString(bytes_sent) + " bytes to client " + Utils::intToString(_fd));
    } else if (bytes_sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::debug("Write error to client " + Utils::intToString(_fd));
        }
    }
    _bytes_sent = bytes_sent;
    return bytes_sent;
}

void Client::clearReadBuffer() {
    _read_buffer.clear();
}

void Client::clearWriteBuffer() {
    _write_buffer.clear();
    _write_offset = 0;
}

void Client::appendToReadBuffer(const std::string& data) {
    _read_buffer.append(data);
}

bool Client::isTimedOut() const {
    return (time(NULL) - _last_activity) > CLIENT_TIMEOUT;
}

bool Client::isWriteComplete() const {
    return _write_offset >= _write_buffer.size() && _bytes_sent > 0;
}

bool Client::hasDataToWrite() const {
    return !_write_buffer.empty() && _write_offset < _write_buffer.size();
}

