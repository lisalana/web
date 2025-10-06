#include "Epoll.hpp"

EpollManager::EpollManager(int flags) : failed(false)
{
	_ep_fd = epoll_create1(flags);
	if (_ep_fd == -1)
	{
		Logger::error("fail to epoll_create1()");
		failed = true;
		return ;
	}

	_events = new epoll_t[MAX_EVENTS]();
	if (!_events)
	{
		Logger::error("fail to allocate epoll_t events");
		failed = true;
		return ;
	}
}

bool EpollManager::isTracked(int fd) const throw()
{
	return fds_callbacks.find(fd) != fds_callbacks.end();
}

bool EpollManager::isTracked(int fd, int event) throw()
{
	if (!isTracked(fd))
		return false;
	return fds_callbacks[fd].find(event) != fds_callbacks[fd].end();
}

int EpollManager::getTrackedEvents(int fd) throw()
{
	int events = 0;
	if (!isTracked(fd))
		return 0;

	for (int i = EVENT_READ; i < __EVENT_COUNTS__; i++)
	{
		if (isTracked(fd, i))
			events |= i;
	}

	return events;
}

bool EpollManager::bindToFd(int fd, uint32_t event, callback_t callback)
{
	epoll_t tmp;

	if (!isTracked(fd))
	{
		fds_callbacks[fd] = std::map<int, callback_t>();
	}
	else if (isTracked(fd, event))
	{
		Logger::warning("fd already bound for this event");
		return true;
	}

	fds_callbacks[fd][event] = callback;

	tmp.data.fd = fd;
	tmp.events = getTrackedEvents(fd) | event;

	if (epoll_ctl(_ep_fd, EPOLL_CTL_MOD, fd, &tmp) == -1)
	{
		epoll_ctl(_ep_fd, EPOLL_CTL_ADD, fd, &tmp);
	}
	return true;
}

bool EpollManager::unbindFd(int fd, int event) throw()
{
	if (event == -1)
	{
		fds_callbacks[fd].clear();
		epoll_ctl(_ep_fd, EPOLL_CTL_DEL, fd, NULL);
		return true;
	}
	if (!isTracked(fd, event))
	{
		Logger::warning("fd is not bound for this event");
		return false;
	}
	else
	{
		epoll_t tmp;

		tmp.data.fd = fd;
		tmp.events = getTrackedEvents(fd) & ~event;

		fds_callbacks[fd].erase(event);
		epoll_ctl(_ep_fd, EPOLL_CTL_MOD, fd, &tmp);
		return true;
	}
}

bool EpollManager::watchForEvents(void *ptr) throw()
{
	int ready = epoll_wait(_ep_fd, _events, MAX_EVENTS, 0);

	if (ready == -1)
	{
		Logger::warning("epoll_wait() fails");
		return false;
	}

	if (ready == 0)
		return true;

	for (size_t i = 0; i < (size_t)ready; i++)
	{
		int fd = _events[i].data.fd;
		int events = _events[i].events;
		for (int binding = EVENT_READ; binding < __EVENT_COUNTS__; binding++)
		{
			if (events & binding && isTracked(fd, binding))
			{
				fds_callbacks[fd][binding](fd, ptr);
			}
		}
	}

	return true;
}

EpollManager::~EpollManager()
{
	close(_ep_fd);
	delete[] _events;
}