#include "IOMultiplexer.hpp"
#include <algorithm>
#include <errno.h>
#include <iostream>
#include <string.h>

IOMultiplexer::IOMultiplexer() {}

IOMultiplexer::~IOMultiplexer() {
    // No need to close file descriptors here; that's the responsibility
    // of the owner of the file descriptors
    _pollfds.clear();
    _fdToData.clear();
}

void IOMultiplexer::addFd(int fd, short events, void* data) {
    // Check if fd already exists
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == fd) {
            // Update events if already exists
            _pollfds[i].events = events;
            _fdToData[fd] = data;
            return;
        }
    }
    
    // Add new fd
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    
    _pollfds.push_back(pfd);
    _fdToData[fd] = data;
}

void IOMultiplexer::modifyFd(int fd, short events) {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == fd) {
            _pollfds[i].events = events;
            return;
        }
    }
}

void IOMultiplexer::removeFd(int fd) {
    for (std::vector<struct pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
        if (it->fd == fd) {
            _pollfds.erase(it);
            _fdToData.erase(fd);
            return;
        }
    }
}

int IOMultiplexer::wait(int timeout) {
    if (_pollfds.empty()) {
        return 0;  // No file descriptors to monitor
    }
    
    // Reset revents fields
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        _pollfds[i].revents = 0;
    }
    
    // Call poll() to wait for events
    int result = poll(&_pollfds[0], _pollfds.size(), timeout);
    
    if (result < 0) {
        if (errno != EINTR) {
            std::cerr << "poll() error: " << strerror(errno) << std::endl;
        }
    }
    
    return result;
}

bool IOMultiplexer::isReadReady(int fd) const {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == fd) {
            return (_pollfds[i].revents & POLLIN) != 0;
        }
    }
    return false;
}

bool IOMultiplexer::isWriteReady(int fd) const {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == fd) {
            return (_pollfds[i].revents & POLLOUT) != 0;
        }
    }
    return false;
}

bool IOMultiplexer::hasError(int fd) const {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == fd) {
            return (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
        }
    }
    return false;
}

void* IOMultiplexer::getData(int fd) const {
    std::map<int, void*>::const_iterator it = _fdToData.find(fd);
    if (it != _fdToData.end()) {
        return it->second;
    }
    return NULL;
}

std::vector<int> IOMultiplexer::getActiveFds() const {
    std::vector<int> activeFds;
    
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].revents != 0) {
            activeFds.push_back(_pollfds[i].fd);
        }
    }
    
    return activeFds;
}

size_t IOMultiplexer::size() const {
    return _pollfds.size();
}