#pragma once

#include <string>
#include <map>
#include <vector>

/**
 * @brief Structure to represent a file in a multipart upload
 */
struct UploadedFile {
    std::string name;          // File field name
    std::string filename;      // Original filename
    std::string contentType;   // File content type
    std::string content;       // File content
};

/**
 * @brief Class to parse multipart/form-data uploads
 */
class MultipartParser {
private:
    std::string _boundary;                  // Boundary string
    std::string _body;                      // Request body
    std::map<std::string, std::string> _fields;  // Form fields
    std::vector<UploadedFile> _files;       // Uploaded files

    /**
     * @brief Extract the boundary from Content-Type header
     * 
     * @param contentType Content-Type header value
     * @return std::string Boundary string
     */
    std::string _extractBoundary(const std::string& contentType);
    
    /**
     * @brief Parse a single part of the multipart data
     * 
     * @param part Part content
     */
    void _parsePart(const std::string& part);
    
    /**
     * @brief Parse headers from a part
     * 
     * @param headers Headers section
     * @return std::map<std::string, std::string> Parsed headers
     */
    std::map<std::string, std::string> _parsePartHeaders(const std::string& headers);
    
    /**
     * @brief Parse the Content-Disposition header
     * 
     * @param contentDisposition Content-Disposition header value
     * @return std::map<std::string, std::string> Parsed attributes
     */
    std::map<std::string, std::string> _parseContentDisposition(const std::string& contentDisposition);

public:
    /**
     * @brief Construct a new MultipartParser object
     * 
     * @param contentType Content-Type header value
     * @param body Request body
     */
    MultipartParser(const std::string& contentType, const std::string& body);
    
    /**
     * @brief Destroy the MultipartParser object
     */
    ~MultipartParser();
    
    /**
     * @brief Parse the multipart data
     * 
     * @return bool True if parsing was successful
     */
    bool parse();
    
    /**
     * @brief Get the form fields
     * 
     * @return const std::map<std::string, std::string>& Form fields
     */
    const std::map<std::string, std::string>& getFields() const;
    
    /**
     * @brief Get the uploaded files
     * 
     * @return const std::vector<UploadedFile>& Uploaded files
     */
    const std::vector<UploadedFile>& getFiles() const;
    
    /**
     * @brief Get a specific field value
     * 
     * @param name Field name
     * @return std::string Field value, empty if not found
     */
    std::string getField(const std::string& name) const;
    
    /**
     * @brief Save an uploaded file to disk
     * 
     * @param index File index
     * @param path Path to save to
     * @return bool True if file was saved successfully
     */
    bool saveFile(size_t index, const std::string& path) const;
};