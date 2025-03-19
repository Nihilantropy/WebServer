#include "StringUtils.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cstdio>

// Private constructor to prevent instantiation
StringUtils::StringUtils() {}

// Private destructor
StringUtils::~StringUtils() {}

std::string StringUtils::trimRight(const std::string& str, const std::string& trimChars)
{
    std::string::size_type pos = str.find_last_not_of(trimChars);
    if (pos == std::string::npos)
        return ""; // If all spaces, return empty string
    return str.substr(0, pos + 1);
}

std::string StringUtils::trimLeft(const std::string& str, const std::string& trimChars)
{
    std::string::size_type pos = str.find_first_not_of(trimChars);
    if (pos == std::string::npos)
        return ""; // If all spaces, return empty string
    return str.substr(pos);
}

std::string StringUtils::trim(const std::string& str, const std::string& trimChars)
{
    return trimLeft(trimRight(str, trimChars), trimChars);
}

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter))
    {
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos)
            tokens.push_back(token.substr(start, end - start + 1));
    }
    return tokens;
}

std::string StringUtils::extractDirectiveValue(const std::string& line, const std::string& directiveName)
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
    value = trim(value);
    
    // Remove trailing semicolon if present
    if (!value.empty() && value[value.length() - 1] == ';')
        value = value.substr(0, value.length() - 1);
    
    // Trim again after removing semicolon
    value = trim(value);
    
    return value;
}

std::string StringUtils::urlDecode(const std::string& str)
{
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.size())
        {
            // Convert hex digits to character
            std::string hex = str.substr(i + 1, 2);
            int value;
            std::istringstream iss(hex);
            iss >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }
    
    return result;
}

std::string StringUtils::urlEncode(const std::string& str)
{
    std::string result;
    result.reserve(str.size() * 3); // Worst case: every char needs encoding
    
    for (size_t i = 0; i < str.size(); ++i)
    {
        unsigned char c = str[i];
        
        // Allow only alphanumeric characters and a few special ones
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            result += c;
        }
        else if (c == ' ')
        {
            result += '+';
        }
        else
        {
            // Percent-encode other characters
            char buf[4];
            sprintf(buf, "%%%02X", c);
            result += buf;
        }
    }
    
    return result;
}

std::string StringUtils::toLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string StringUtils::toUpper(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool StringUtils::equalsIgnoreCase(const std::string& str1, const std::string& str2)
{
    if (str1.size() != str2.size())
        return false;
        
    for (size_t i = 0; i < str1.size(); ++i)
    {
        if (tolower(str1[i]) != tolower(str2[i]))
            return false;
    }
    
    return true;
}

bool StringUtils::contains(const std::string& str, const std::string& subStr, bool caseSensitive)
{
    if (caseSensitive)
    {
        return str.find(subStr) != std::string::npos;
    }
    else
    {
        std::string lowerStr = toLower(str);
        std::string lowerSubStr = toLower(subStr);
        return lowerStr.find(lowerSubStr) != std::string::npos;
    }
}

std::string StringUtils::replace(const std::string& str, const std::string& from, const std::string& to)
{
    std::string result = str;
    size_t pos = 0;
    
    while ((pos = result.find(from, pos)) != std::string::npos)
    {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return result;
}