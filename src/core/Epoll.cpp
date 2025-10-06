#include "Epoll.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <cstring>

Epoll::Epoll() : _epoll_fd(-1) {
    _ready_events.reserve(MAX_EVENTS);
}

Epoll::~Epoll() {
    if (_epoll_fd != -1) {
        close(_epoll_fd);
    }
}

bool Epoll::init() {
    _epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (_epoll_fd == -1) {
        Logger::error("Failed to create epoll fd");
        return false;
    }
    Logger::debug("Epoll initialized successfully");
    return true;
}

bool Epoll::addFd(int fd, EventType events, void* data) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = eventTypeToEpollEvents(events);
    ev.data.ptr = data;
    ev.data.fd = fd;
    
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        Logger::error("Failed to add fd to epoll");
        return false;
    }
    
    Logger::debug("Added fd " + Utils::intToString(fd) + " to epoll");
    return true;
}

bool Epoll::modifyFd(int fd, EventType events, void* data) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = eventTypeToEpollEvents(events);
    ev.data.ptr = data;
    ev.data.fd = fd;
    
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        Logger::error("Failed to modify fd in epoll");
        return false;
    }
    
    return true;
}

bool Epoll::removeFd(int fd) {
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        Logger::error("Failed to remove fd from epoll");
        return false;
    }
    
    Logger::debug("Removed fd " + Utils::intToString(fd) + " from epoll");
    return true;
}

int Epoll::wait(int timeout_ms) {
    _ready_events.clear();
    
    int nfds = epoll_wait(_epoll_fd, _events, MAX_EVENTS, timeout_ms);
    if (nfds == -1) {
        Logger::error("epoll_wait failed");
        return -1;
    }
    
    // Convert epoll events to our Event structure
    for (int i = 0; i < nfds; i++) {
        Event event;
        event.fd = _events[i].data.fd;
        event.data = _events[i].data.ptr;
        
        // Determine event type
        if (_events[i].events & EPOLLIN) {
            event.type = EVENT_READ;
        } else if (_events[i].events & EPOLLOUT) {
            event.type = EVENT_WRITE;
        } else if (_events[i].events & (EPOLLERR | EPOLLHUP)) {
            event.type = EVENT_ERROR;
        }
        
        _ready_events.push_back(event);
    }
    
    return nfds;
}

const std::vector<Event>& Epoll::getReadyEvents() const {
    return _ready_events;
}

bool Epoll::isValid() const {
    return _epoll_fd != -1;
}

uint32_t Epoll::eventTypeToEpollEvents(EventType type) {
    switch (type) {
        case EVENT_READ:
            return EPOLLIN | EPOLLET;
        case EVENT_WRITE:
            return EPOLLOUT | EPOLLET;
        case EVENT_ERROR:
            return EPOLLERR | EPOLLHUP;
        default:
            return EPOLLIN;
    }
}