#pragma once

#include <string>
#include <vector>

class LocationConfig
{
public:
	LocationConfig();
	~LocationConfig();

	const std::string&					getPath( void ) const;
	const std::string&					getRoot( void ) const;
	const std::vector<std::string>&		getAllowedMethods( void ) const;
	const std::string&					getIndex( void ) const;
	const std::string&					getAutoIndex( void ) const;
	const std::string&					getCgiPath( void ) const;
	const std::vector<std::string>&		getCgiExtentions( void ) const;
	const std::string&					getUploadDir( void ) const;
	const std::string&					getRedirection( void ) const;

	void	setPath( const std::string& );
	void	setRoot( const std::string& );
	void	setAllowedMethods();
	void	setIndex( const std::string& );
	void	setAutoIndex();
	void	setCgiPath( const std::string& );
	void	setCgiExtentions();
	void	setUploadDir( const std::string& );
	void	setRedirection( const std::string& );

	void	parseLocationBlock( std::istream& file );

private:

	std::string					path;
	std::string					root;
	std::vector<std::string>	allowedMethods;
	std::string					index;
	bool						autoIndex;
	std::string					cgiPath;
	std::vector<std::string>	cgiExtentions;
	std::string					uploadDir;
	std::string					redirection;

	LocationConfig( const LocationConfig& other );
	LocationConfig& operator=( const LocationConfig& other );
};