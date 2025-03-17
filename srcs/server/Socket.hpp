#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include "../exceptions/exceptions.hpp"

class Socket
{
private:
	int					_socketFd;         // Socket file descriptor
	std::string			_host;             // IP address to bind to
	int					_port;             // Port to bind to
	struct sockaddr_in	_address;       // Socket address structure
	bool				_isNonBlocking;    // Flag for non-blocking mode

public:
	Socket(const std::string& host, int port);
	~Socket();

	void	create();                  // Create the socket
	void	setNonBlocking();          // Set socket to non-blocking mode
	void	bind();                    // Bind socket to address and port
	void	listen(int backlog = 10);  // Start listening for connections
	int		accept();                  // Accept a new connection
	void	close();                   // Close the socket

	// Getters
	int					getSocketFd() const;
	const std::string&	getHost() const;
	int					getPort() const;
	bool				isNonBlocking() const;

private:
	// Prevent copying
	Socket(const Socket& other);
	Socket& operator=(const Socket& other);
};