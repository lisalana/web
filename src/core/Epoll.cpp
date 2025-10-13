/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rguigneb <rguigneb@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/13 16:23:43 by rguigneb          #+#    #+#             */
/*   Updated: 2025/10/13 16:23:44 by rguigneb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"
#include "Utils.hpp"
#include <cerrno>
#include <cstring>
#include <sstream>

namespace {

std::string eventMaskToString(uint32_t events) {
	std::string names;

	if (events & EVENT_READ) {
		names += "READ";
	}
	if (events & EVENT_WRITE) {
		if (!names.empty())
			names += "|";
		names += "WRITE";
	}
	if (events & EVENT_ERROR) {
		if (!names.empty())
			names += "|";
		names += "ERROR";
	}
	if (names.empty()) {
		std::ostringstream oss;
		oss << "0x" << std::hex << events;
		names = oss.str();
	}
	return names;
}

}

EpollManager::EpollManager(int flags) : failed(false)
{
	Logger::debug("Initializing epoll (flags=" + Utils::intToString(flags) + ", READ="
		+ eventMaskToString(EVENT_READ) + ", WRITE=" + eventMaskToString(EVENT_WRITE)
		+ ", ERROR=" + eventMaskToString(EVENT_ERROR) + ")");

	_ep_fd = epoll_create1(flags);
	if (_ep_fd == -1)
	{
		Logger::error("epoll_create1 failed: " + std::string(strerror(errno)));
		failed = true;
		return ;
	}

	_events = new epoll_t[MAX_EVENTS]();
	if (!_events)
	{
		Logger::error("Failed to allocate epoll events buffer");
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
		Logger::debug("fd " + Utils::intToString(fd) + " already bound for event " + eventMaskToString(event));
		return true;
	}

	fds_callbacks[fd][event] = callback;

	tmp.data.fd = fd;
	tmp.events = getTrackedEvents(fd) | event;

	if (epoll_ctl(_ep_fd, EPOLL_CTL_MOD, fd, &tmp) == -1)
	{
		epoll_ctl(_ep_fd, EPOLL_CTL_ADD, fd, &tmp);
	}
	Logger::debug("Bound fd " + Utils::intToString(fd) + " for event " + eventMaskToString(event));
	return true;
}

bool EpollManager::unbindFd(int fd, int event) throw()
{
	if (event == -1)
	{
		fds_callbacks[fd].clear();
		epoll_ctl(_ep_fd, EPOLL_CTL_DEL, fd, NULL);
		Logger::debug("Unbound all events for fd " + Utils::intToString(fd));
		return true;
	}
	if (!isTracked(fd, event))
	{
		Logger::debug("fd " + Utils::intToString(fd) + " is not bound for event " + eventMaskToString(event));
		return false;
	}
	else
	{
		epoll_t tmp;

		tmp.data.fd = fd;
		tmp.events = getTrackedEvents(fd) & ~event;

		fds_callbacks[fd].erase(event);
		epoll_ctl(_ep_fd, EPOLL_CTL_MOD, fd, &tmp);
		Logger::debug("Unbound fd " + Utils::intToString(fd) + " for event " + eventMaskToString(event));
		return true;
	}
}

bool EpollManager::watchForEvents(void *ptr) throw()
{
	int ready = epoll_wait(_ep_fd, _events, MAX_EVENTS, 0);

	if (ready == -1)
	{
		Logger::error("epoll_wait failed: " + std::string(strerror(errno)));
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
