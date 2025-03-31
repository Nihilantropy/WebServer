#pragma once

#include <vector>
#include <map>
#include <sys/poll.h>
#include <cstddef>

/**
 * @brief Wrapper class for poll() to manage multiple file descriptors
 * 
 * This class encapsulates the poll() functionality and provides a clean API
 * for non-blocking I/O operations. It allows monitoring multiple file descriptors
 * for read and write readiness without blocking the server.
 */
class IOMultiplexer {
private:
    std::vector<struct pollfd> _pollfds;
    std::map<int, void*> _fdToData;
    
public:
    // Event types (matching poll() flags)
    static const short EVENT_READ = POLLIN;
    static const short EVENT_WRITE = POLLOUT;
    static const short EVENT_ERROR = POLLERR | POLLHUP | POLLNVAL;
    
    /**
     * @brief Construct a new IOMultiplexer object
     */
    IOMultiplexer();
    
    /**
     * @brief Destroy the IOMultiplexer object
     */
    ~IOMultiplexer();
    
    /**
     * @brief Add a file descriptor to monitor
     * 
     * @param fd The file descriptor to monitor
     * @param events Event flags (EVENT_READ, EVENT_WRITE, etc.)
     * @param data Pointer to associated data (will be returned by getData())
     */
    void addFd(int fd, short events, void* data);
    
    /**
     * @brief Update the events to monitor for a file descriptor
     * 
     * @param fd The file descriptor to update
     * @param events New event flags
     */
    void modifyFd(int fd, short events);
    
    /**
     * @brief Remove a file descriptor from monitoring
     * 
     * @param fd The file descriptor to remove
     */
    void removeFd(int fd);
    
    /**
     * @brief Wait for events on monitored file descriptors
     * 
     * @param timeout Timeout in milliseconds, -1 for indefinite
     * @return int Number of file descriptors with events, 0 for timeout, -1 for error
     */
    int wait(int timeout = -1);
    
    /**
     * @brief Check if a file descriptor is ready for reading
     * 
     * @param fd The file descriptor to check
     * @return true if ready for reading, false otherwise
     */
    bool isReadReady(int fd) const;
    
    /**
     * @brief Check if a file descriptor is ready for writing
     * 
     * @param fd The file descriptor to check
     * @return true if ready for writing, false otherwise
     */
    bool isWriteReady(int fd) const;
    
    /**
     * @brief Check if a file descriptor has an error
     * 
     * @param fd The file descriptor to check
     * @return true if error occurred, false otherwise
     */
    bool hasError(int fd) const;
    
    /**
     * @brief Get the data associated with a file descriptor
     * 
     * @param fd The file descriptor
     * @return void* Pointer to associated data, NULL if not found
     */
    void* getData(int fd) const;
    
    /**
     * @brief Get all file descriptors with activity
     * 
     * @return std::vector<int> List of active file descriptors
     */
    std::vector<int> getActiveFds() const;
    
    /**
     * @brief Get the current number of monitored file descriptors
     * 
     * @return size_t Number of monitored file descriptors
     */
    size_t size() const;
    
private:
    // Prevent copying
    IOMultiplexer(const IOMultiplexer& other);
    IOMultiplexer& operator=(const IOMultiplexer& other);
};