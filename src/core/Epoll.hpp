#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>
#include <vector>
#include <cstddef>

#define MAX_EVENTS 1024

enum EventType {
    EVENT_READ = EPOLLIN,
    EVENT_WRITE = EPOLLOUT,
    EVENT_ERROR = EPOLLERR | EPOLLHUP
};

struct Event {
    int fd;
    EventType type;
    void* data;
    
    Event() : fd(-1), type(EVENT_READ), data(0) {}
    Event(int f, EventType t, void* d = 0) : fd(f), type(t), data(d) {
        (void)d;
    }
};

class Epoll {
private:
    int _epoll_fd;
    struct epoll_event _events[MAX_EVENTS];
    std::vector<Event> _ready_events;

public:
    Epoll();
    ~Epoll();

    bool init();
    bool addFd(int fd, EventType events, void* data = 0);
    bool modifyFd(int fd, EventType events, void* data = 0);
    bool removeFd(int fd);
    
    int wait(int timeout_ms = -1);
    const std::vector<Event>& getReadyEvents() const;
    
    bool isValid() const;

private:
    uint32_t eventTypeToEpollEvents(EventType type);
};

#endif