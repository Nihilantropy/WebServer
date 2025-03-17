#include "Socket.hpp"

Socket::Socket(const std::string& host, int port)
    : _socketFd(-1), _host(host), _port(port), _isNonBlocking(false)
{
    std::memset(&_address, 0, sizeof(_address));
}

Socket::~Socket()
{
    close();
}

void Socket::create()
{
    // Create a TCP socket
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd < 0)
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    
    // Allow reuse of local addresses
    int opt = 1;
    if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close();
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }
}

void Socket::setNonBlocking()
{
    // Set socket to non-blocking mode using fcntl
    // This is critical for our WebServer requirements
    int flags = fcntl(_socketFd, F_GETFL, 0);
    if (flags < 0)
    {
        close();
        throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
    }
    
    if (fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        close();
        throw std::runtime_error("Failed to set socket to non-blocking mode: " + std::string(strerror(errno)));
    }
    
    _isNonBlocking = true;
}

void Socket::bind()
{
    // Set up the address structure
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    
    // Convert host string to network address
    if (_host == "0.0.0.0" || _host == "localhost")
        _address.sin_addr.s_addr = INADDR_ANY;
    else if (inet_pton(AF_INET, _host.c_str(), &_address.sin_addr) <= 0)
    {
        close();
        throw std::runtime_error("Invalid address: " + _host);
    }
    
    // Bind the socket to the address
    if (::bind(_socketFd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        close();
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
}

void Socket::listen(int backlog)
{
    // Start listening for connections
    if (::listen(_socketFd, backlog) < 0)
    {
        close();
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
}

int Socket::accept()
{
    // Accept a new connection
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    int clientFd = ::accept(_socketFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    
    // We don't throw on EAGAIN/EWOULDBLOCK because these aren't errors
    // in non-blocking mode, they just mean "no connection available right now"
    return clientFd;
}

void Socket::close()
{
    if (_socketFd >= 0)
    {
        ::close(_socketFd);
        _socketFd = -1;
    }
}

// Getters
int Socket::getSocketFd() const { return _socketFd; }
const std::string& Socket::getHost() const { return _host; }
int Socket::getPort() const { return _port; }
bool Socket::isNonBlocking() const { return _isNonBlocking; }