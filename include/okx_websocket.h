#ifndef OKX_WEBSOCKET_H
#define OKX_WEBSOCKET_H

#include "data_types.h"
#include "okx_signer.h"
#include "nlohmann/json.hpp"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <map>
#include <set>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> WebSocketClient;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

/**
 * @brief OKX WebSocket client for real-time market data and private channels
 * 
 * Features:
 * - Public channels: tickers, depth, trades, candlesticks
 * - Private channels: orders, positions, account, balance
 * - Automatic reconnection with exponential backoff
 * - Heartbeat/ping-pong mechanism
 * - Subscription management
 * - Thread-safe callbacks
 * 
 * WebSocket Documentation: https://www.okx.com/docs-v5/en/#overview-websocket
 */
class OKXWebSocket {
public:
    // Callback types
    using TickCallback = std::function<void(const Tick&)>;
    using DepthCallback = std::function<void(const Depth&)>;
    using OrderCallback = std::function<void(const Order&)>;
    using PositionCallback = std::function<void(const Position&)>;
    using AccountCallback = std::function<void(const Account&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    struct WSConfig {
        std::string url = "wss://ws.okx.com:8443/ws/v5/public";
        std::string private_url = "wss://ws.okx.com:8443/ws/v5/private";
        
        // Authentication (for private channels)
        std::string api_key;
        std::string secret_key;
        std::string passphrase;
        
        // Reconnection settings
        bool auto_reconnect = true;
        int reconnect_delay_seconds = 5;
        int max_reconnect_attempts = 10;
        
        // Ping settings
        int ping_interval_seconds = 20;
    };
    
public:
    OKXWebSocket();
    ~OKXWebSocket();
    
    /**
     * @brief Initialize WebSocket
     */
    bool Initialize(const WSConfig& config);
    
    /**
     * @brief Connect to WebSocket server
     */
    bool Connect();
    
    /**
     * @brief Disconnect from WebSocket server
     */
    void Disconnect();
    
    /**
     * @brief Check if connected
     */
    bool IsConnected() const;
    
    // ==================== Public Channel Subscriptions ====================
    
    /**
     * @brief Subscribe to ticker channel
     * @param inst_id Instrument ID (e.g., "XAUT-USDT-SWAP")
     * @param callback Callback function for tick updates
     */
    bool SubscribeTicker(const std::string& inst_id, TickCallback callback);
    
    /**
     * @brief Subscribe to orderbook (depth) channel
     * @param inst_id Instrument ID
     * @param callback Callback function for depth updates
     * @param depth_type "books" for full depth, "books5" for top 5 levels
     */
    bool SubscribeDepth(const std::string& inst_id, 
                        DepthCallback callback,
                        const std::string& depth_type = "books5");
    
    /**
     * @brief Unsubscribe from ticker
     */
    bool UnsubscribeTicker(const std::string& inst_id);
    
    /**
     * @brief Unsubscribe from depth
     */
    bool UnsubscribeDepth(const std::string& inst_id);
    
    // ==================== Private Channel Subscriptions ====================
    
    /**
     * @brief Subscribe to orders channel (requires authentication)
     * @param inst_id Instrument ID (empty for all instruments)
     * @param callback Callback function for order updates
     */
    bool SubscribeOrders(const std::string& inst_id, OrderCallback callback);
    
    /**
     * @brief Subscribe to positions channel
     * @param inst_id Instrument ID (empty for all instruments)
     * @param callback Callback function for position updates
     */
    bool SubscribePositions(const std::string& inst_id, PositionCallback callback);
    
    /**
     * @brief Subscribe to account channel
     * @param callback Callback function for account updates
     */
    bool SubscribeAccount(AccountCallback callback);
    
    /**
     * @brief Unsubscribe from private channels
     */
    bool UnsubscribeOrders(const std::string& inst_id);
    bool UnsubscribePositions(const std::string& inst_id);
    bool UnsubscribeAccount();
    
    // ==================== Error Handling ====================
    
    /**
     * @brief Set error callback
     */
    void SetErrorCallback(ErrorCallback callback);
    
    /**
     * @brief Get connection statistics
     */
    struct Statistics {
        uint64_t total_messages_received = 0;
        uint64_t total_messages_sent = 0;
        uint64_t reconnection_count = 0;
        uint64_t subscription_count = 0;
        bool is_connected = false;
        std::chrono::steady_clock::time_point last_message_time;
    };
    
    Statistics GetStatistics() const;
    
private:
    // WebSocket event handlers
    void OnOpen(websocketpp::connection_hdl hdl);
    void OnClose(websocketpp::connection_hdl hdl);
    void OnFail(websocketpp::connection_hdl hdl);
    void OnMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg);
    
    // SSL/TLS context handler
    context_ptr OnTLSInit(websocketpp::connection_hdl hdl);
    
    // Message processing
    void ProcessMessage(const std::string& message);
    void ProcessTickerMessage(const json& data);
    void ProcessDepthMessage(const json& data);
    void ProcessOrderMessage(const json& data);
    void ProcessPositionMessage(const json& data);
    void ProcessAccountMessage(const json& data);
    
    // Subscription management
    bool SendSubscription(const std::string& channel, const std::string& inst_id = "");
    bool SendUnsubscription(const std::string& channel, const std::string& inst_id = "");
    
    // Authentication
    bool Authenticate();
    std::string GenerateAuthSignature(const std::string& timestamp);
    
    // Reconnection logic
    void ReconnectionLoop();
    bool AttemptReconnect();
    
    // Ping/Pong mechanism
    void PingLoop();
    void SendPing();
    
    // Helper functions
    void RunEventLoop();
    void StopEventLoop();
    
private:
    WSConfig config_;
    std::unique_ptr<OKXSigner> signer_;
    
    // WebSocket client
    WebSocketClient ws_client_;
    websocketpp::connection_hdl connection_hdl_;
    bool is_connected_;
    bool should_reconnect_;
    
    // Threading
    std::unique_ptr<std::thread> event_thread_;
    std::unique_ptr<std::thread> ping_thread_;
    std::unique_ptr<std::thread> reconnect_thread_;
    
    // Callbacks
    std::map<std::string, TickCallback> ticker_callbacks_;
    std::map<std::string, DepthCallback> depth_callbacks_;
    std::map<std::string, OrderCallback> order_callbacks_;
    std::map<std::string, PositionCallback> position_callbacks_;
    AccountCallback account_callback_;
    ErrorCallback error_callback_;
    
    // Subscription tracking
    std::set<std::string> active_subscriptions_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

#endif // OKX_WEBSOCKET_H
