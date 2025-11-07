#include "okx_signer.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <chrono>

OKXSigner::OKXSigner(const std::string& api_key,
                     const std::string& secret_key,
                     const std::string& passphrase)
    : api_key_(api_key)
    , secret_key_(secret_key)
    , passphrase_(passphrase) {
}

std::string OKXSigner::Sign(const std::string& timestamp,
                            const std::string& method,
                            const std::string& request_path,
                            const std::string& body) const {
    // OKX signature format: timestamp + method + requestPath + body
    std::string prehash = timestamp + method + request_path + body;
    
    // Calculate HMAC-SHA256
    std::string hmac = HMACSHA256(secret_key_, prehash);
    
    // Encode to Base64
    return Base64Encode(reinterpret_cast<const unsigned char*>(hmac.c_str()), hmac.length());
}

std::string OKXSigner::GetTimestamp() {
    using namespace std::chrono;
    
    // Get current time with milliseconds
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = system_clock::to_time_t(now);
    
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &timer);
#else
    gmtime_r(&timer, &tm);
#endif
    
    // Format: 2023-01-01T12:00:00.123Z
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return oss.str();
}

std::string OKXSigner::HMACSHA256(const std::string& key, const std::string& data) const {
    unsigned char* digest;
    unsigned int len = SHA256_DIGEST_LENGTH;
    
    digest = HMAC(EVP_sha256(),
                  key.c_str(), key.length(),
                  reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
                  nullptr, nullptr);
    
    return std::string(reinterpret_cast<char*>(digest), len);
}

std::string OKXSigner::Base64Encode(const unsigned char* data, size_t length) const {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data, length);
    BIO_flush(bio);
    
    BIO_get_mem_ptr(bio, &buffer_ptr);
    
    std::string result(buffer_ptr->data, buffer_ptr->length);
    
    BIO_free_all(bio);
    
    return result;
}
