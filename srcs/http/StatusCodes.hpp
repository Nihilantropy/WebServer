#pragma once

#include <string>
#include <map>

// HTTP status codes
#define HTTP_STATUS_OK                    200
#define HTTP_STATUS_CREATED               201
#define HTTP_STATUS_ACCEPTED              202
#define HTTP_STATUS_NO_CONTENT            204

#define HTTP_STATUS_MOVED_PERMANENTLY     301
#define HTTP_STATUS_FOUND                 302
#define HTTP_STATUS_SEE_OTHER             303
#define HTTP_STATUS_NOT_MODIFIED          304
#define HTTP_STATUS_TEMPORARY_REDIRECT    307
#define HTTP_STATUS_PERMANENT_REDIRECT    308

#define HTTP_STATUS_BAD_REQUEST           400
#define HTTP_STATUS_UNAUTHORIZED          401
#define HTTP_STATUS_FORBIDDEN             403
#define HTTP_STATUS_NOT_FOUND             404
#define HTTP_STATUS_METHOD_NOT_ALLOWED    405
#define HTTP_STATUS_REQUEST_TIMEOUT       408
#define HTTP_STATUS_LENGTH_REQUIRED       411
#define HTTP_STATUS_PAYLOAD_TOO_LARGE     413

#define HTTP_STATUS_INTERNAL_SERVER_ERROR 500
#define HTTP_STATUS_NOT_IMPLEMENTED       501
#define HTTP_STATUS_BAD_GATEWAY           502
#define HTTP_STATUS_SERVICE_UNAVAILABLE   503
#define HTTP_STATUS_GATEWAY_TIMEOUT       504
#define HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED 505

/**
 * @brief Get the reason phrase for an HTTP status code
 * 
 * @param statusCode HTTP status code
 * @return std::string Reason phrase, or "Unknown Status" if not found
 */
std::string getReasonPhrase(int statusCode);

/**
 * @brief Get a static map of status codes to reason phrases
 * 
 * @return const std::map<int, std::string>& Map of status codes to reason phrases
 */
const std::map<int, std::string>& getStatusCodesMap();