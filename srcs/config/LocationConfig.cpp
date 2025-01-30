#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
	: _path(NULL), _root(NULL), _allowedMethods(NULL), _index(NULL),
		_autoIndex(false), _cgiPath(NULL), _cgiExtentions(NULL), _uploadDir(NULL), _redirection(NULL) {}

LocationConfig::~LocationConfig() {}

/*** private ***/
LocationConfig::LocationConfig( const LocationConfig& other ) { (void)other; }

LocationConfig&	LocationConfig::operator=( const LocationConfig& other )
{
	(void)other;
	return *this;
}

/*** Getter ***/
const std::string&					LocationConfig::getPath( void ) const { return _path; }
const std::string&					LocationConfig::getRoot( void ) const { return _root; }
const std::vector<std::string>&		LocationConfig::getAllowedMethods( void ) const { return _allowedMethods; }
const std::string&					LocationConfig::getIndex( void ) const { return _index; }
const bool&							LocationConfig::getAutoIndex( void ) const { return _autoIndex; }
const std::string&					LocationConfig::getCgiPath( void ) const { return _cgiPath; }
const std::vector<std::string>&		LocationConfig::getCgiExtentions( void ) const { return _cgiExtentions; }
const std::string&					LocationConfig::getUploadDir( void ) const { return _uploadDir; }
const std::string&					LocationConfig::getRedirection( void ) const { return _redirection; }

/*** Setter ***/
void	LocationConfig::setPath( const std::string& path ) { _path = path; }
void	LocationConfig::setRoot( const std::string& root ) { _root = root; }
void	LocationConfig::setAllowedMethods( std::vector<std::string>& allowedMethods ) { _allowedMethods = allowedMethods; }
void	LocationConfig::setIndex( const std::string& index ) { _index = index; }
void	LocationConfig::setAutoIndex( const bool& autoIndex ) { _autoIndex = autoIndex; }
void	LocationConfig::setCgiPath( const std::string& cgiPath ) { _cgiPath = cgiPath; }
void	LocationConfig::setCgiExtentions( std::vector<std::string>& cgiExtentions ) { _cgiExtentions = cgiExtentions; }
void	LocationConfig::setUploadDir( const std::string& uploadDir ) { _uploadDir = uploadDir; }
void	LocationConfig::setRedirection( const std::string& redirection ) { _redirection = redirection; }

/*** private helper methods ***/

/**
 * @brief private method to add an allowed method string to the class _allowedMethods vector
 * 
 * @param allowedMethod a string containign the allowed method (GET, POST or DELETE)
 */
void	LocationConfig::addAllowedMethod( const std::string& allowedMethod )
{
	_allowedMethods.push_back(allowedMethod);
}

/**
 * @brief private method to add a cgi extention string to the class _cgiExtentions vector
 * 
 * @param cgiExtention a string containign the cgi extention (.php, .py, .sh etc...)
 */
void	LocationConfig::addCgiExtention( const std::string& cgiExtention )
{
	_cgiExtentions.push_back(cgiExtention);
}

/**
 * @brief parser method to get all location-info from .conf file
 */
void	LocationConfig::parseLocationBlock( const std::istream& file )
{

}