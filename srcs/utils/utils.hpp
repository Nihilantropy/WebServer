#pragma once

#include <string>
#include <vector>
#include <sstream>

const std::string	whiteSpaces( " \f\n\r\t\v" );

enum e_space
{
	SPACE = ' ',
	TAB = '\t'
};

std::string	trim( const std::string& str, const std::string& trimChars );
std::string	trimRight( const std::string& str, const std::string& trimChars );
std::string	trimLeft( const std::string& str, const std::string& trimChars );

std::vector<std::string>	split( const std::string& str, char delimiter );