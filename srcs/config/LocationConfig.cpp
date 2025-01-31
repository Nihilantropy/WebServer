#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
	: _path(), _root(), _allowedMethods(), _index(),
		_autoIndex(false), _cgiPath(), _cgiExtentions(), _uploadDir(), _redirection() {}

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
void	LocationConfig::_addAllowedMethod( const std::string& allowedMethod )
{
	_allowedMethods.push_back(allowedMethod);
}

/**
 * @brief private method to add a cgi extention string to the class _cgiExtentions vector
 * 
 * @param cgiExtention a string containign the cgi extention (.php, .py, .sh etc...)
 */
void	LocationConfig::_addCgiExtention( const std::string& cgiExtention )
{
	_cgiExtentions.push_back(cgiExtention);
}

/**
 * @brief parser method to get all location-info from .conf file
 */
void LocationConfig::parseLocationBlock( std::ifstream& file )
{
    std::string line;

    while (std::getline(file, line))
	{
        line = trim(line, whiteSpaces);
        if (line.empty() || line[0] == '#')
			continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "root")
            setRoot(line);
        else if (key == "allowed_methods")
		{
            std::vector<std::string> methods = split(line, SPACE);
            setAllowedMethods(methods);
        }
		else if (key == "index")
            setIndex(line);
        else if (key == "autoindex")
            setAutoIndex(line == "on");
        else if (key == "cgi_extension")
		{
            std::vector<std::string> extensions = split(line, SPACE);
            setCgiExtentions(extensions);
        }
		else if (key == "cgi_path")
            setCgiPath(line);
		else if (key == "upload_dir")
            setUploadDir(line);
        else if (key == "return")
            setRedirection(line);
        else if (key == "}")
            return;
        else
            throw ConfigException("Unknown directive inside 'location': " + key);
    }

    throw ConfigException("Missing closing '}' for location block.");
}

/**
 * @brief << operator overload
 */
std::ostream&	operator<<( std::ostream& os, const LocationConfig& location )
{
	os << "            LocationConfig {" << std::endl;
	os << "                Path: " << location.getPath() << std::endl;
	os << "                Root: " << location.getRoot() << std::endl;

	os << "                Allowed Methods: ";
	for (std::vector<std::string>::const_iterator it = location.getAllowedMethods().begin(); it != location.getAllowedMethods().end(); ++it)
		os << *it << " ";
	os << std::endl;

	os << "                Index: " << location.getIndex() << std::endl;
	os << "                AutoIndex: " << location.getAutoIndex() << std::endl;
	os << "                CGI Path: " << location.getCgiPath() << std::endl;

	os << "                CGI Extensions: ";
	for (std::vector<std::string>::const_iterator it = location.getCgiExtentions().begin(); it != location.getCgiExtentions().end(); ++it)
		os << *it << " ";
	os << std::endl;

	os << "                Upload Directory: " << location.getUploadDir() << std::endl;
	os << "                Redirection: " << location.getRedirection() << std::endl;

	os << "            }" << std::endl;
	return os;
}