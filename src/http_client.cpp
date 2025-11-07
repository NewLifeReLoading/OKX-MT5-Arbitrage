#include "http_client.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>

HttpClient::HttpClient() 
    : curl_(nullptr) {
}

HttpClient::~HttpClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

bool HttpClient::Initialize(const RequestOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    options_ = options;
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
    
    if (!curl_) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return false;
    }
    
    // Set default options
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, options_.timeout_ms);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, options_.connect_timeout_ms);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, options_.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, options_.verify_ssl ? 1L : 0L);
    
    // Enable keep-alive for connection reuse
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 60L);
    
    // Proxy settings
    if (!options_.proxy_url.empty()) {
        curl_easy_setopt(curl_, CURLOPT_PROXY, options_.proxy_url.c_str());
    }
    
    return true;
}

HttpClient::Response HttpClient::Get(const std::string& url, 
                                     const std::map<std::string, std::string>& headers) {
    return PerformRequest("GET", url, "", headers);
}

HttpClient::Response HttpClient::Post(const std::string& url, 
                                      const std::string& body,
                                      const std::map<std::string, std::string>& headers) {
    return PerformRequest("POST", url, body, headers);
}

HttpClient::Response HttpClient::Delete(const std::string& url,
                                        const std::map<std::string, std::string>& headers) {
    return PerformRequest("DELETE", url, "", headers);
}

HttpClient::Response HttpClient::Put(const std::string& url,
                                     const std::string& body,
                                     const std::map<std::string, std::string>& headers) {
    return PerformRequest("PUT", url, body, headers);
}

void HttpClient::SetDefaultHeaders(const std::map<std::string, std::string>& headers) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_headers_ = headers;
}

void HttpClient::ResetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = Statistics();
}

HttpClient::Response HttpClient::PerformRequest(const std::string& method,
                                                const std::string& url,
                                                const std::string& body,
                                                const std::map<std::string, std::string>& headers) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Response response;
    response.status_code = 0;
    
    if (!curl_) {
        response.body = "CURL not initialized";
        return response;
    }
    
    // Rate limiting
    RateLimit();
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Retry logic
    int attempts = 0;
    bool success = false;
    
    while (attempts < options_.max_retries && !success) {
        attempts++;
        
        // Reset response
        response.body.clear();
        response.headers.clear();
        
        // Set URL
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        
        // Set method
        if (method == "GET") {
            curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        } else if (method == "POST") {
            curl_easy_setopt(curl_, CURLOPT_POST, 1L);
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, body.size());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
        } else if (method == "PUT") {
            curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, body.size());
        }
        
        // Set headers
        struct curl_slist* chunk = nullptr;
        
        // Add default headers
        for (const auto& [key, value] : default_headers_) {
            std::string header = key + ": " + value;
            chunk = curl_slist_append(chunk, header.c_str());
        }
        
        // Add custom headers (override defaults)
        for (const auto& [key, value] : headers) {
            std::string header = key + ": " + value;
            chunk = curl_slist_append(chunk, header.c_str());
        }
        
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, chunk);
        
        // Set callbacks
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.body);
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response.headers);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        
        // Clean up headers
        curl_slist_free_all(chunk);
        
        if (res == CURLE_OK) {
            // Get response code
            long http_code = 0;
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
            response.status_code = static_cast<int>(http_code);
            
            success = true;
            stats_.successful_requests++;
        } else {
            response.body = curl_easy_strerror(res);
            response.status_code = 0;
            
            // Exponential backoff for retry
            if (attempts < options_.max_retries) {
                int delay_ms = 100 * (1 << attempts); // 100ms, 200ms, 400ms, ...
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    response.response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    // Update statistics
    stats_.total_requests++;
    if (!success) {
        stats_.failed_requests++;
    }
    stats_.total_bytes_sent += body.size();
    stats_.total_bytes_received += response.body.size();
    
    // Update average response time
    double total_time = stats_.avg_response_time_ms * (stats_.total_requests - 1);
    stats_.avg_response_time_ms = (total_time + response.response_time_ms) / stats_.total_requests;
    
    return response;
}

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

size_t HttpClient::HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_size = size * nitems;
    std::string header(buffer, total_size);
    
    // Parse header
    size_t colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header.substr(0, colon_pos);
        std::string value = header.substr(colon_pos + 1);
        
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);
        
        auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
        (*headers)[key] = value;
    }
    
    return total_size;
}

void HttpClient::RateLimit() {
    if (options_.max_requests_per_second <= 0) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto min_interval = std::chrono::milliseconds(1000 / options_.max_requests_per_second);
    
    if (last_request_time_.time_since_epoch().count() > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_request_time_);
        
        if (elapsed < min_interval) {
            std::this_thread::sleep_for(min_interval - elapsed);
        }
    }
    
    last_request_time_ = std::chrono::steady_clock::now();
}
