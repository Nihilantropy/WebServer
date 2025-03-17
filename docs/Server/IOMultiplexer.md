# Detailed Explanation of the IOMultiplexer Class

## 1. IOMultiplexer Class Purpose and Overview

The IOMultiplexer class is a wrapper around the POSIX poll() system call that centralizes all I/O multiplexing operations in one place. This class is critical for meeting the project's requirements to use only one poll() (or equivalent) for all I/O operations and to ensure the server never blocks. It allows the server to:

- Monitor multiple file descriptors simultaneously for read/write readiness
- Associate data with file descriptors for easy lookup
- Handle I/O operations efficiently without blocking
- Detect errors on file descriptors

## 2. Class Attributes Explained

```cpp
private:
    std::vector<struct pollfd> _pollfds;
    std::map<int, void*> _fdToData;
```

- **_pollfds**: A vector of pollfd structures used by the poll() system call. Each structure contains:
  - **fd**: The file descriptor to monitor
  - **events**: Bit mask of events to monitor (POLLIN for read, POLLOUT for write)
  - **revents**: Bit mask of events that occurred (filled by poll())

- **_fdToData**: A map that associates each file descriptor with arbitrary data (like Socket or Connection objects). This allows for easy lookup of the object associated with a file descriptor when an event occurs.

## 3. Constructor and Destructor

```cpp
IOMultiplexer::IOMultiplexer() {}

IOMultiplexer::~IOMultiplexer() {
    // No need to close file descriptors here; that's the responsibility
    // of the owner of the file descriptors
    _pollfds.clear();
    _fdToData.clear();
}
```

- **Constructor**: Simple initialization with no parameters.
  
- **Destructor**: Clears internal data structures but does not close file descriptors. This is an important design decision – the class doesn't own the file descriptors, it only monitors them. The responsibility for closing file descriptors remains with the objects that created them (Socket and Connection classes).

## 4. Core Multiplexing Functions

### addFd()

```cpp
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
```

- **Purpose**: Adds a file descriptor to be monitored by poll()
- **Parameters**:
  - **fd**: The file descriptor to monitor
  - **events**: Bit mask of events to monitor (EVENT_READ, EVENT_WRITE)
  - **data**: Pointer to associated data (usually a Socket or Connection object)
- **Behavior**:
  - First checks if the file descriptor is already being monitored
  - If it exists, updates the events and associated data
  - If it doesn't exist, creates a new pollfd structure and adds it to the vector
  - Initializes revents to 0 (will be set by poll() when events occur)
  - Stores the associated data in the map for later retrieval

### modifyFd()

```cpp
void IOMultiplexer::modifyFd(int fd, short events) {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == fd) {
            _pollfds[i].events = events;
            return;
        }
    }
}
```

- **Purpose**: Updates the events to monitor for an existing file descriptor
- **Parameters**:
  - **fd**: The file descriptor to update
  - **events**: New bit mask of events to monitor
- **Behavior**:
  - Searches for the file descriptor in the vector
  - If found, updates the events field
  - If not found, does nothing (silent fail)
- **Usage**: Typically used to switch between monitoring for read, write, or both, based on the current state of a connection

### removeFd()

```cpp
void IOMultiplexer::removeFd(int fd) {
    for (std::vector<struct pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
        if (it->fd == fd) {
            _pollfds.erase(it);
            _fdToData.erase(fd);
            return;
        }
    }
}
```

- **Purpose**: Removes a file descriptor from monitoring
- **Parameters**:
  - **fd**: The file descriptor to remove
- **Behavior**:
  - Searches for the file descriptor in the vector
  - If found, erases it from both the vector and the map
  - If not found, does nothing (silent fail)
- **Usage**: Called when a connection is closed or a socket is no longer needed

### wait()

```cpp
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
```

- **Purpose**: The core function that calls poll() to wait for events on monitored file descriptors
- **Parameters**:
  - **timeout**: Maximum time to wait in milliseconds (-1 for indefinite)
- **Return value**:
  - **> 0**: Number of file descriptors with events
  - **0**: Timeout expired without events
  - **-1**: Error occurred
- **Behavior**:
  - If no file descriptors are being monitored, returns 0 immediately
  - Resets all revents fields to 0 before calling poll()
  - Calls poll() with the current set of file descriptors
  - Logs errors except for EINTR (which happens when interrupted by a signal)
- **Key aspects**:
  - This is the only place in the code that calls poll(), centralizing all I/O multiplexing
  - The timeout parameter allows for periodic tasks like checking for connection timeouts
  - After this call, revents fields will be filled with any events that occurred

## 5. Event Detection Functions

### isReadReady(), isWriteReady(), hasError()

```cpp
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
```

- **Purpose**: Check if specific events occurred on a file descriptor
- **Parameters**:
  - **fd**: The file descriptor to check
- **Return value**: Boolean indicating if the event occurred
- **Behavior**:
  - Searches for the file descriptor in the vector
  - Checks the revents field against appropriate event flags
  - Returns false if the file descriptor is not found
- **Usage patterns**:
  - `isReadReady()`: Called to check if data is available to read
  - `isWriteReady()`: Called to check if the socket is ready for writing
  - `hasError()`: Called to check for error conditions (connection closed, error occurred)

### getData()

```cpp
void* IOMultiplexer::getData(int fd) const {
    std::map<int, void*>::const_iterator it = _fdToData.find(fd);
    if (it != _fdToData.end()) {
        return it->second;
    }
    return NULL;
}
```

- **Purpose**: Retrieves the data associated with a file descriptor
- **Parameters**:
  - **fd**: The file descriptor to look up
- **Return value**: Pointer to associated data, or NULL if not found
- **Usage**: Used to get the Socket or Connection object associated with a file descriptor that has activity

### getActiveFds()

```cpp
std::vector<int> IOMultiplexer::getActiveFds() const {
    std::vector<int> activeFds;
    
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].revents != 0) {
            activeFds.push_back(_pollfds[i].fd);
        }
    }
    
    return activeFds;
}
```

- **Purpose**: Gets a list of all file descriptors with activity
- **Return value**: Vector of file descriptors that have any events
- **Behavior**:
  - Iterates through all pollfd structures
  - Adds file descriptors with non-zero revents to the result vector
- **Usage**: Used in the main event loop to efficiently process only file descriptors with activity

### size()

```cpp
size_t IOMultiplexer::size() const {
    return _pollfds.size();
}
```

- **Purpose**: Returns the number of file descriptors being monitored
- **Return value**: Count of monitored file descriptors
- **Usage**: Useful for monitoring the number of active connections

## 6. Key Design Decisions

### 1. Separation of Concerns

The IOMultiplexer class focuses solely on multiplexing I/O operations, without knowing the details of what those file descriptors represent. This separation allows:
- Clear responsibility boundaries
- Easier testing and maintenance
- Flexibility in how file descriptors are used

### 2. Ownership Model

The class doesn't take ownership of file descriptors:
- It doesn't create or close file descriptors
- It only monitors them while they're active
- The responsibility for cleanup remains with the Socket and Connection classes

### 3. Data Association

The void* association pattern allows:
- Storing any type of object with a file descriptor
- Easy lookup when events occur
- No template dependencies that would complicate the design

### 4. Error Handling

The class logs errors but generally doesn't throw exceptions:
- Makes it more robust in a server environment
- Allows the server to continue running even if some operations fail
- Provides error information where it's useful

## 7. Why This Design Matters for Our Web Server

- **Centralized I/O Multiplexing**: Satisfies the project requirement to use only one poll() for all I/O operations
- **Non-blocking Design**: Ensures the server never blocks on I/O operations
- **Efficient Event Processing**: Allows the server to process only file descriptors with activity
- **Clean API**: Provides a simple interface for the Server class to monitor multiple connections
- **Readiness Checking**: Ensures we only perform I/O operations when we know they won't block

This design is crucial for building a high-performance web server that can handle many concurrent connections without dedicating a thread to each one, which is a key requirement for the WebServer project.