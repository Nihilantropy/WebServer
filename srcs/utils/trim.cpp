#include "utils.hpp"

/**
 * @brief Trims whitespace from the right end of a string.
 * 
 * @param str The input string.
 * @param trimChars Characters to trim (default: whitespace).
 * @return std::string The trimmed string.
 */
std::string	trimRight( const std::string& str, const std::string& trimChars )
{
	std::string::size_type pos = str.find_last_not_of(trimChars);
	if (pos == std::string::npos)
		return ""; // If all spaces, return empty string
	return str.substr(0, pos + 1);
}

/**
 * @brief Trims whitespace from the left end of a string.
 * 
 * @param str The input string.
 * @param trimChars Characters to trim (default: whitespace).
 * @return std::string The trimmed string.
 */
std::string	trimLeft(const std::string& str, const std::string& trimChars)
{
	std::string::size_type pos = str.find_first_not_of(trimChars);
	if (pos == std::string::npos)
		return ""; // If all spaces, return empty string
	return str.substr(pos);
}

/**
 * @brief Trims whitespace from both ends of a string.
 * 
 * @param str The input string.
 * @param trimChars Characters to trim (default: whitespace).
 * @return std::string The fully trimmed string.
 */
std::string	trim( const std::string& str, const std::string& trimChars )
{
	return trimLeft(trimRight(str, trimChars), trimChars);
}
