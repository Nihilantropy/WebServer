#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include "../../exceptions/exceptions.hpp"
#include "../../utils/StringUtils.hpp"

#define DEFAULT_CLIENT_SIZE static_cast<size_t>(-1) // Use server's value

/**
 * @brief class to store location-specific info
 */
class LocationConfig
{
public:
	LocationConfig();
	~LocationConfig();

	const std::string&					getPath( void ) const;
	const std::string&					getRoot( void ) const;
	const std::vector<std::string>&		getAllowedMethods( void ) const;
	const size_t&						getClientMaxBodySize( void ) const;
	const std::string&					getIndex( void ) const;
	const bool&							getAutoIndex( void ) const;
	const std::string&					getCgiPath( void ) const;
	const std::vector<std::string>&		getCgiExtentions( void ) const;
	const std::map<std::string, std::string>& getCgiHandlers( void ) const;
	const std::string&					getUploadDir( void ) const;
	const std::string&					getRedirection( void ) const;

	void	setPath( const std::string& path );
	void	setRoot( const std::string& root );
	void	setAllowedMethods( std::vector<std::string>& allowedMethods );
	void	setClientMaxBodySize(const size_t& clientMaxBodySize);
	void	setIndex( const std::string& index );
	void	setAutoIndex( const bool& autoIndex );
	void	setCgiPath( const std::string& cgiPath );
	void	setCgiExtentions( std::vector<std::string>& cgiExtentions );
	void	setCgiHandlers( std::map<std::string, std::string>& cgiHandlers );
	void	setUploadDir( const std::string& uploadDir );
	void	setRedirection( const std::string& redirection );

	void	parseLocationBlock( std::ifstream& file );

	// Get the interpreter for a specific extension
	std::string getInterpreterForExtension(const std::string& extension) const;

private:

	std::string					_path;
	std::string					_root;
	std::vector<std::string>	_allowedMethods;
	size_t						_clientMaxBodySize;  // Client max body size in bytes (SIZE_MAX means inherit from server)
	std::string					_index;
	bool						_autoIndex;
	std::string					_cgiPath;            // Legacy: Default CGI path (deprecated, use _cgiHandlers instead)
	std::vector<std::string>	_cgiExtentions;      // Legacy: List of CGI extensions
	std::map<std::string, std::string> _cgiHandlers; // Map of extension to interpreter path
	std::string					_uploadDir;
	std::string					_redirection;

	void	_addAllowedMethod( const std::string& allowedMethod );
	size_t	_parseSize(const std::string& sizeStr);
	void	_addCgiExtention( const std::string& cgiExtention );
	void	_addCgiHandler( const std::string& extension, const std::string& interpreter );
	void	_parseCgiHandlerDirective( const std::string& directive );

	LocationConfig( const LocationConfig& other );
	LocationConfig& operator=( const LocationConfig& other );
};

std::ostream&	operator<<(std::ostream& os, const LocationConfig& location);