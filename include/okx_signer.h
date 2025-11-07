#ifndef OKX_SIGNER_H
#define OKX_SIGNER_H

#include <string>
#include <ctime>

/**
 * @brief OKX API signature generator
 * 
 * OKX API requires HMAC-SHA256 signature for authentication:
 * signature = base64(HMAC-SHA256(SecretKey, timestamp + method + requestPath + body))
 */
class OKXSigner {
public:
    /**
     * @brief Constructor
     * @param api_key API Key
     * @param secret_key Secret Key
     * @param passphrase Passphrase
     */
    OKXSigner(const std::string& api_key, 
              const std::string& secret_key,
              const std::string& passphrase);
    
    /**
     * @brief Generate signature for OKX API request
     * @param timestamp ISO 8601 timestamp (e.g., "2023-01-01T12:00:00.123Z")
     * @param method HTTP method (GET, POST, etc.)
     * @param request_path Request path (e.g., "/api/v5/trade/order")
     * @param body Request body (empty for GET requests)
     * @return Base64 encoded signature
     */
    std::string Sign(const std::string& timestamp,
                    const std::string& method,
                    const std::string& request_path,
                    const std::string& body = "") const;
    
    /**
     * @brief Get current ISO 8601 timestamp
     * @return Timestamp string (e.g., "2023-01-01T12:00:00.123Z")
     */
    static std::string GetTimestamp();
    
    /**
     * @brief Get API key
     */
    const std::string& GetAPIKey() const { return api_key_; }
    
    /**
     * @brief Get passphrase
     */
    const std::string& GetPassphrase() const { return passphrase_; }

private:
    /**
     * @brief Calculate HMAC-SHA256
     */
    std::string HMACSHA256(const std::string& key, const std::string& data) const;
    
    /**
     * @brief Encode to Base64
     */
    std::string Base64Encode(const unsigned char* data, size_t length) const;

private:
    std::string api_key_;
    std::string secret_key_;
    std::string passphrase_;
};

#endif // OKX_SIGNER_H
