#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "../../exceptions/exceptions.hpp"
#include "../../utils/StringUtils.hpp"

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
	const std::string&					getIndex( void ) const;
	const bool&							getAutoIndex( void ) const;
	const std::string&					getCgiPath( void ) const;
	const std::vector<std::string>&		getCgiExtentions( void ) const;
	const std::string&					getUploadDir( void ) const;
	const std::string&					getRedirection( void ) const;

	void	setPath( const std::string& path );
	void	setRoot( const std::string& root );
	void	setAllowedMethods( std::vector<std::string>& allowedMethods );
	void	setIndex( const std::string& index );
	void	setAutoIndex( const bool& autoIndex );
	void	setCgiPath( const std::string& cgiPath );
	void	setCgiExtentions( std::vector<std::string>& cgiExtentions );
	void	setUploadDir( const std::string& uploadDir );
	void	setRedirection( const std::string& redirection );

	void	parseLocationBlock( std::ifstream& file );

private:

	std::string					_path;
	std::string					_root;
	std::vector<std::string>	_allowedMethods;
	std::string					_index;
	bool						_autoIndex;
	std::string					_cgiPath;
	std::vector<std::string>	_cgiExtentions;
	std::string					_uploadDir;
	std::string					_redirection;

	void	_addAllowedMethod( const std::string& allowedMethod );
	void	_addCgiExtention( const std::string& cgiExtention );

	LocationConfig( const LocationConfig& other );
	LocationConfig& operator=( const LocationConfig& other );
};

std::ostream&	operator<<(std::ostream& os, const LocationConfig& location);