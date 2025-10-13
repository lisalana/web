/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rguigneb <rguigneb@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/13 16:24:54 by rguigneb          #+#    #+#             */
/*   Updated: 2025/10/13 16:24:55 by rguigneb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>
#include <vector>
#include <cstddef>
#include <sys/epoll.h>
#include <unistd.h>
#include <map>
#include "Logger.hpp"

#define MAX_EVENTS 1024

enum EventType {
    EVENT_READ = EPOLLIN,
    EVENT_WRITE = EPOLLOUT,
    EVENT_ERROR = EPOLLERR | EPOLLHUP,
    __EVENT_COUNTS__
};

typedef struct epoll_event epoll_t;
typedef int fd_t;

class EpollManager
{

	public:
		typedef void (*callback_t)(int, void *);

	private:

		epoll_t *_events;

		std::map<int, std::map<int, callback_t> > fds_callbacks;

		fd_t _ep_fd;

        
    public:

        bool failed;

		EpollManager(int flags);

		bool isTracked(int fd) const throw();

		bool isTracked(int fd, int event) throw();

		int getTrackedEvents(int fd) throw();

		bool bindToFd(int fd, uint32_t event, callback_t callback);

		bool unbindFd(int fd, int event) throw();

		bool watchForEvents(void *ptr) throw();

		~EpollManager();
};


#endif