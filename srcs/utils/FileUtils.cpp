#include "FileUtils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <iomanip>
#include <cstring>

bool FileUtils::fileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool FileUtils::isDirectory(const std::string& path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return S_ISDIR(buffer.st_mode);
}

std::string FileUtils::getFileContents(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

size_t FileUtils::getFileSize(const std::string& path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return 0;
    }
    return buffer.st_size;
}

std::string FileUtils::getFileExtension(const std::string& path)
{
    size_t dotPos = path.find_last_of(".");
    if (dotPos == std::string::npos) {
        return "";
    }
    return path.substr(dotPos + 1);
}

std::string FileUtils::getMimeType(const std::string& extension)
{
    std::string ext = extension;
    
    // Remove leading dot if present
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    
    // Convert to lowercase
    for (size_t i = 0; i < ext.length(); ++i) {
        ext[i] = tolower(ext[i]);
    }
    
    const std::map<std::string, std::string>& mimeTypes = getMimeTypes();
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
    
    if (it != mimeTypes.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

const std::map<std::string, std::string>& FileUtils::getMimeTypes()
{
    static std::map<std::string, std::string> mimeTypes;
    
    // Initialize on first call
    if (mimeTypes.empty()) {
        // Text
        mimeTypes["html"] = "text/html";
        mimeTypes["htm"] = "text/html";
        mimeTypes["css"] = "text/css";
        mimeTypes["js"] = "text/javascript";
        mimeTypes["txt"] = "text/plain";
        mimeTypes["md"] = "text/markdown";
        mimeTypes["csv"] = "text/csv";
        
        // Images
        mimeTypes["gif"] = "image/gif";
        mimeTypes["jpg"] = "image/jpeg";
        mimeTypes["jpeg"] = "image/jpeg";
        mimeTypes["png"] = "image/png";
        mimeTypes["svg"] = "image/svg+xml";
        mimeTypes["ico"] = "image/x-icon";
        mimeTypes["webp"] = "image/webp";
        
        // Audio
        mimeTypes["mp3"] = "audio/mpeg";
        mimeTypes["wav"] = "audio/wav";
        mimeTypes["ogg"] = "audio/ogg";
        
        // Video
        mimeTypes["mp4"] = "video/mp4";
        mimeTypes["webm"] = "video/webm";
        
        // Applications
        mimeTypes["json"] = "application/json";
        mimeTypes["xml"] = "application/xml";
        mimeTypes["pdf"] = "application/pdf";
        mimeTypes["zip"] = "application/zip";
        mimeTypes["gz"] = "application/gzip";
        mimeTypes["tar"] = "application/x-tar";
        
        // Fonts
        mimeTypes["ttf"] = "font/ttf";
        mimeTypes["woff"] = "font/woff";
        mimeTypes["woff2"] = "font/woff2";
    }
    
    return mimeTypes;
}

std::string FileUtils::generateDirectoryListing(const std::string& dirPath, const std::string& requestPath)
{
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        return "";
    }
    
    std::stringstream html;
    html << "<!DOCTYPE html>\r\n"
         << "<html>\r\n"
         << "<head>\r\n"
         << "    <title>Index of " << requestPath << "</title>\r\n"
         << "    <style>\r\n"
         << "        body { font-family: Arial, sans-serif; margin: 20px; }\r\n"
         << "        h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }\r\n"
         << "        table { border-collapse: collapse; width: 100%; }\r\n"
         << "        th, td { text-align: left; padding: 8px; }\r\n"
         << "        tr:nth-child(even) { background-color: #f2f2f2; }\r\n"
         << "        th { background-color: #4CAF50; color: white; }\r\n"
         << "        a { text-decoration: none; color: #0066cc; }\r\n"
         << "        a:hover { text-decoration: underline; }\r\n"
         << "    </style>\r\n"
         << "</head>\r\n"
         << "<body>\r\n"
         << "    <h1>Index of " << requestPath << "</h1>\r\n"
         << "    <table>\r\n"
         << "        <tr>\r\n"
         << "            <th>Name</th>\r\n"
         << "            <th>Last Modified</th>\r\n"
         << "            <th>Size</th>\r\n"
         << "        </tr>\r\n";
    
    // Add parent directory link (except for root)
    if (requestPath != "/") {
        html << "        <tr>\r\n"
             << "            <td><a href=\"../\">Parent Directory</a></td>\r\n"
             << "            <td>-</td>\r\n"
             << "            <td>-</td>\r\n"
             << "        </tr>\r\n";
    }
    
    // Read all directory entries
    std::vector<std::string> files;
    std::vector<std::string> directories;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        // Skip hidden files and the current/parent directory entries
        if (name[0] == '.' && (name.length() == 1 || (name.length() == 2 && name[1] == '.'))) {
            continue;
        }
        
        // Skip other hidden files
        if (name[0] == '.') {
            continue;
        }
        
        // Check if it's a directory
        std::string fullPath = dirPath + "/" + name;
        if (isDirectory(fullPath)) {
            directories.push_back(name);
        } else {
            files.push_back(name);
        }
    }
    
    closedir(dir);
    
    // Sort directories and files
    std::sort(directories.begin(), directories.end());
    std::sort(files.begin(), files.end());
    
    // Add directories first
    for (std::vector<std::string>::const_iterator it = directories.begin(); it != directories.end(); ++it) {
        std::string fullPath = dirPath + "/" + *it;
        struct stat buffer;
        stat(fullPath.c_str(), &buffer);
        
        // Format modification time
        std::stringstream modTime;
        struct tm* timeinfo = localtime(&buffer.st_mtime);
        modTime << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
        
        html << "        <tr>\r\n"
             << "            <td><a href=\"" << *it << "/\">" << *it << "/</a></td>\r\n"
             << "            <td>" << modTime.str() << "</td>\r\n"
             << "            <td>-</td>\r\n"
             << "        </tr>\r\n";
    }
    
    // Add files
    for (std::vector<std::string>::const_iterator it = files.begin(); it != files.end(); ++it) {
        std::string fullPath = dirPath + "/" + *it;
        struct stat buffer;
        stat(fullPath.c_str(), &buffer);
        
        // Format modification time
        std::stringstream modTime;
        struct tm* timeinfo = localtime(&buffer.st_mtime);
        modTime << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
        
        // Format size
        std::string sizeStr;
        if (buffer.st_size < 1024) {
            sizeStr = std::to_string(buffer.st_size) + " B";
        } else if (buffer.st_size < 1024 * 1024) {
            sizeStr = std::to_string(buffer.st_size / 1024) + " KB";
        } else {
            sizeStr = std::to_string(buffer.st_size / (1024 * 1024)) + " MB";
        }
        
        html << "        <tr>\r\n"
             << "            <td><a href=\"" << *it << "\">" << *it << "</a></td>\r\n"
             << "            <td>" << modTime.str() << "</td>\r\n"
             << "            <td>" << sizeStr << "</td>\r\n"
             << "        </tr>\r\n";
    }
    
    html << "    </table>\r\n"
         << "    <hr>\r\n"
         << "    <p>WebServer</p>\r\n"
         << "</body>\r\n"
         << "</html>\r\n";
    
    return html.str();
}

std::string FileUtils::resolvePath(const std::string& uriPath, const LocationConfig& location)
{
    std::string locationPath = location.getPath();
    std::string root = location.getRoot();
    
    // Ensure location path ends with a slash
    if (!locationPath.empty() && locationPath[locationPath.length() - 1] != '/') {
        locationPath += '/';
    }
    
    // Ensure root ends with a slash
    if (!root.empty() && root[root.length() - 1] != '/') {
        root += '/';
    }
    
    // If the URI exactly matches the location path
    if (uriPath == location.getPath()) {
        return root;
    }
    
    // Check if the URI starts with the location path
    if (uriPath.find(locationPath) == 0) {
        // Extract the part after the location path
        std::string relativePath = uriPath.substr(locationPath.length());
        
        // Concatenate with root
        return root + relativePath;
    }
    
    // Default case: Just append the URI to the root
    // This is a fallback and should not normally be reached
    return root + uriPath;
}