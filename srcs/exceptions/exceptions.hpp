#pragma once

#include <stdexcept>

class OpenException : public std::exception
{
public:
	explicit OpenException( const char* msg )
		: _msg(std::string("Error opening file: ") + msg) {}

	virtual ~OpenException() throw() {}

	const char*	what() const throw()
	{
		return _msg.c_str();
	}

private:
	std::string	_msg;
};

class ConfigException : public std::runtime_error
{
public:
	explicit ConfigException( const std::string& msg )
		: std::runtime_error("Config Error: " + msg) {}
};

class ValidationException : public std::runtime_error
{
public:
	explicit ValidationException( const std::string& msg )
		: std::runtime_error("Validation exception: " + msg) {}
};
