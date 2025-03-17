#include "utils.hpp"

/**
 * @brief Extract the value part of a directive, removing the directive name, comments, and trimming
 * 
 * @param line The full line containing the directive
 * @param directiveName The name of the directive to remove
 * @return std::string The cleaned value
 */
 std::string extractDirectiveValue(const std::string& line, const std::string& directiveName)
 {
	 // Find the position after the directive name
	 size_t pos = line.find(directiveName);
	 if (pos == std::string::npos)
		 return "";
		 
	 pos += directiveName.length();
	 
	 // Get the rest of the line
	 std::string value = line.substr(pos);
	 
	 // Remove comments (everything after #)
	 size_t commentPos = value.find('#');
	 if (commentPos != std::string::npos)
		 value = value.substr(0, commentPos);
	 
	 // Trim whitespace
	 value = trim(value, whiteSpaces);
	 
	 // Remove trailing semicolon if present
	 if (!value.empty() && value[value.length() - 1] == ';')
		 value = value.substr(0, value.length() - 1);
	 
	 // Trim again after removing semicolon
	 value = trim(value, whiteSpaces);
	 
	 return value;
 }