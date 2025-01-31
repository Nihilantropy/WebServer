#include "utils.hpp"

/**
 * @brief Splits a string into tokens based on a delimiter.
 */
std::vector<std::string> split( const std::string& str, char delimiter )
{
	std::vector<std::string>	tokens;
	std::istringstream			iss(str);
	std::string					token;

	while (std::getline(iss, token, delimiter))
	{
		size_t start = token.find_first_not_of(" \t");
		size_t end = token.find_last_not_of(" \t");
		if (start != std::string::npos)
			tokens.push_back(token.substr(start, end - start + 1));
	}
	return tokens;
}