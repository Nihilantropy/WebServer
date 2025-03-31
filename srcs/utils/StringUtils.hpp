#pragma once

#include <string>
#include <vector>
#include <map>

/**
 * @brief Class containing string manipulation utility functions
 */
class StringUtils
{
public:
    /**
     * @brief Trims whitespace from the right end of a string.
     * 
     * @param str The input string.
     * @param trimChars Characters to trim (default: whitespace).
     * @return std::string The trimmed string.
     */
    static std::string trimRight(const std::string& str, const std::string& trimChars = " \t\n\r\f\v");

    /**
     * @brief Trims whitespace from the left end of a string.
     * 
     * @param str The input string.
     * @param trimChars Characters to trim (default: whitespace).
     * @return std::string The trimmed string.
     */
    static std::string trimLeft(const std::string& str, const std::string& trimChars = " \t\n\r\f\v");

    /**
     * @brief Trims whitespace from both ends of a string.
     * 
     * @param str The input string.
     * @param trimChars Characters to trim (default: whitespace).
     * @return std::string The fully trimmed string.
     */
    static std::string trim(const std::string& str, const std::string& trimChars = " \t\n\r\f\v");

    /**
     * @brief Splits a string into tokens based on a delimiter.
     * 
     * @param str The input string.
     * @param delimiter The character to split on.
     * @return std::vector<std::string> Vector of tokens.
     */
    static std::vector<std::string> split(const std::string& str, char delimiter);

    /**
     * @brief Extract the value part of a directive, removing the directive name, comments, and trimming
     * 
     * @param line The full line containing the directive
     * @param directiveName The name of the directive to remove
     * @return std::string The cleaned value
     */
    static std::string extractDirectiveValue(const std::string& line, const std::string& directiveName);

    /**
     * @brief URL decode a string (convert %XX to characters)
     * 
     * @param str String to decode
     * @return std::string Decoded string
     */
    static std::string urlDecode(const std::string& str);

    /**
     * @brief URL encode a string (convert special characters to %XX)
     * 
     * @param str String to encode
     * @return std::string Encoded string
     */
    static std::string urlEncode(const std::string& str);

    /**
     * @brief Convert a string to lowercase
     * 
     * @param str Input string
     * @return std::string Lowercase string
     */
    static std::string toLower(const std::string& str);

    /**
     * @brief Convert a string to uppercase
     * 
     * @param str Input string
     * @return std::string Uppercase string
     */
    static std::string toUpper(const std::string& str);

    /**
     * @brief Compare two strings case-insensitively
     * 
     * @param str1 First string
     * @param str2 Second string
     * @return bool True if strings are equal ignoring case
     */
    static bool equalsIgnoreCase(const std::string& str1, const std::string& str2);

    /**
     * @brief Check if a string contains a substring
     * 
     * @param str String to search in
     * @param subStr Substring to search for
     * @param caseSensitive Whether to perform case-sensitive search
     * @return bool True if substring is found
     */
    static bool contains(const std::string& str, const std::string& subStr, bool caseSensitive = true);

    /**
     * @brief Replace all occurrences of a substring
     * 
     * @param str Input string
     * @param from Substring to replace
     * @param to Replacement string
     * @return std::string String with replacements
     */
    static std::string replace(const std::string& str, const std::string& from, const std::string& to);

private:
    // Private constructor to prevent instantiation
    StringUtils();
    // Private destructor
    ~StringUtils();
    // Private copy constructor and assignment operator to prevent copying
    StringUtils(const StringUtils& other);
    StringUtils& operator=(const StringUtils& other);
};