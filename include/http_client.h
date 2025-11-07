#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <map>
#include <functional>
#include <curl/curl.h>
#include <memory>
#include <chrono>    // ← 添加这个
#include <mutex>     // ← 添加这个

/**
 * @brief High-performance HTTP client with connection pooling
 * 
 * Features:
 * - Connection reuse (keep-alive)
 * - Automatic retry with exponential backoff
 * - Timeout management
 * - Custom headers support
 * - Thread-safe
 */
class HttpClient {
public:
    struct Response {
        int status_code;
        std::string body;
        std::map<std::string, std::string> headers;
        long response_time_ms;  // Response time in milliseconds
        
        bool IsSuccess() const { return status_code >= 200 && status_code < 300; }
    };
    
    struct RequestOptions {
        int timeout_ms;                      // Default 5 seconds
        int connect_timeout_ms;              // Connection timeout
        int max_retries;                     // Retry on failure
        bool verify_ssl;                     // SSL verification
        bool follow_redirects;
        std::map<std::string, std::string> headers;
        
        // Rate limiting
        int max_requests_per_second;
        
        // Proxy settings (optional)
        std::string proxy_url;
        
        // Constructor with defaults
        RequestOptions() 
            : timeout_ms(5000)
            , connect_timeout_ms(3000)
            , max_retries(3)
            , verify_ssl(true)
            , follow_redirects(true)
            , max_requests_per_second(10)
        {}
    };

public:
    HttpClient();
    ~HttpClient();
    
    // Disable copy
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    /**
     * @brief Initialize the HTTP client
     * @param options Request options
     * @return true if successful
     */
    bool Initialize(const RequestOptions& options = RequestOptions());
    
    /**
     * @brief Make a GET request
     * @param url The URL to request
     * @param headers Custom headers (optional)
     * @return Response object
     */
    Response Get(const std::string& url, 
                 const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Make a POST request
     * @param url The URL to request
     * @param body Request body
     * @param headers Custom headers (optional)
     * @return Response object
     */
    Response Post(const std::string& url, 
                  const std::string& body,
                  const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Make a DELETE request
     */
    Response Delete(const std::string& url,
                    const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Make a PUT request
     */
    Response Put(const std::string& url,
                 const std::string& body,
                 const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Set default headers for all requests
     */
    void SetDefaultHeaders(const std::map<std::string, std::string>& headers);
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        uint64_t total_requests = 0;
        uint64_t successful_requests = 0;
        uint64_t failed_requests = 0;
        uint64_t total_bytes_sent = 0;
        uint64_t total_bytes_received = 0;
        double avg_response_time_ms = 0.0;
    };
    
    Statistics GetStatistics() const { return stats_; }
    void ResetStatistics();
    
private:
    Response PerformRequest(const std::string& method,
                           const std::string& url,
                           const std::string& body,
                           const std::map<std::string, std::string>& headers);
    
    // Callback for libcurl to write response data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    
    // Apply rate limiting
    void RateLimit();
    
private:
    CURL* curl_;
    RequestOptions options_;
    std::map<std::string, std::string> default_headers_;
    Statistics stats_;
    
    // Rate limiting
    std::chrono::steady_clock::time_point last_request_time_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

#endif // HTTP_CLIENT_H
