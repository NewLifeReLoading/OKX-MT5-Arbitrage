#ifndef OKX_REST_API_H
#define OKX_REST_API_H

#include "http_client.h"
#include "okx_signer.h"
#include "data_types.h"
#include "nlohmann/json.hpp"
#include <memory>
#include <vector>
#include <mutex>     // ← 添加这个

using json = nlohmann::json;

/**
 * @brief Complete OKX REST API implementation
 * 
 * Features:
 * - All public and private endpoints
 * - Complete field mapping (100+ fields)
 * - Automatic retry and error handling
 * - Rate limiting (10 requests/second)
 * - Connection pooling
 * 
 * API Documentation: https://www.okx.com/docs-v5/en/
 */
class OKXRestAPI {
public:
    struct APIConfig {
        std::string base_url = "https://www.okx.com";
        std::string api_key;
        std::string secret_key;
        std::string passphrase;
        bool is_simulation = false;  // true for demo trading
        int timeout_ms = 5000;
        int max_retries = 3;
    };
    
public:
    OKXRestAPI();
    ~OKXRestAPI();
    
    /**
     * @brief Initialize API with configuration
     */
    bool Initialize(const APIConfig& config);
    
    // ==================== Public Market Data ====================
    
    /**
     * @brief Get ticker data
     * @param inst_id Instrument ID (e.g., "XAUT-USDT-SWAP")
     * @return Tick structure
     */
    Tick GetTicker(const std::string& inst_id);
    
    /**
     * @brief Get orderbook (depth)
     * @param inst_id Instrument ID
     * @param depth_size Number of depth levels (1-400)
     * @return Depth structure with bids/asks
     */
    Depth GetOrderBook(const std::string& inst_id, int depth_size = 5);
    
    /**
     * @brief Get funding rate
     * @param inst_id Instrument ID
     * @return Current funding rate
     */
    struct FundingRate {
        std::string inst_id;
        double funding_rate;
        double funding_time;
        double next_funding_rate;
        double next_funding_time;
    };
    FundingRate GetFundingRate(const std::string& inst_id);
    
    /**
     * @brief Get candlestick data
     * @param inst_id Instrument ID
     * @param bar Bar size ("1m", "5m", "15m", "1H", "1D", etc.)
     * @param limit Number of candles (max 300)
     * @return Vector of candlesticks
     */
    struct Candlestick {
        uint64_t timestamp;
        double open;
        double high;
        double low;
        double close;
        double volume;
        double volume_currency;
    };
    std::vector<Candlestick> GetCandlesticks(const std::string& inst_id,
                                             const std::string& bar = "1m",
                                             int limit = 100);
    
    /**
     * @brief Get instrument info
     */
    struct InstrumentInfo {
        std::string inst_id;
        std::string inst_type;  // SPOT, SWAP, FUTURES, OPTION
        std::string underlying;
        std::string base_ccy;
        std::string quote_ccy;
        std::string settle_ccy;
        double contract_val;
        double tick_size;
        double lot_size;
        double min_size;
        std::string state;  // live, suspend, expired
    };
    InstrumentInfo GetInstrumentInfo(const std::string& inst_id);
    
    // ==================== Account API (Private) ====================
    
    /**
     * @brief Get account balance
     * @return Account structure with complete balance info
     */
    Account GetAccountBalance();
    
    /**
     * @brief Get positions
     * @param inst_id Instrument ID (optional, empty for all)
     * @return Vector of positions
     */
    std::vector<Position> GetPositions(const std::string& inst_id = "");
    
    /**
     * @brief Get account configuration
     */
    struct AccountConfig {
        int position_mode;  // 1: long/short mode, 2: net mode
        bool auto_loan;
        int level;  // Account level
        double leverage;
        std::string account_level;  // 1-4
    };
    AccountConfig GetAccountConfig();
    
    /**
     * @brief Set leverage
     * @param inst_id Instrument ID
     * @param leverage Leverage (1-125)
     * @param margin_mode cross or isolated
     */
    bool SetLeverage(const std::string& inst_id, 
                     int leverage,
                     const std::string& margin_mode = "cross");
    
    // ==================== Trading API (Private) ====================
    
    /**
     * @brief Place order
     * @param order Order details
     * @return Order ID if successful, empty string otherwise
     */
    std::string PlaceOrder(const Order& order);
    
    /**
     * @brief Batch place orders (up to 20 orders)
     */
    std::vector<std::string> PlaceBatchOrders(const std::vector<Order>& orders);
    
    /**
     * @brief Cancel order
     * @param inst_id Instrument ID
     * @param order_id Order ID (optional)
     * @param client_order_id Client order ID (optional)
     */
    bool CancelOrder(const std::string& inst_id,
                     const std::string& order_id = "",
                     const std::string& client_order_id = "");
    
    /**
     * @brief Cancel batch orders (up to 20 orders)
     */
    struct CancelRequest {
        std::string inst_id;
        std::string order_id;
        std::string client_order_id;
    };
    std::vector<bool> CancelBatchOrders(const std::vector<CancelRequest>& requests);
    
    /**
     * @brief Amend order (modify pending order)
     */
    bool AmendOrder(const std::string& inst_id,
                   const std::string& order_id,
                   const std::string& new_size = "",
                   const std::string& new_price = "");
    
    /**
     * @brief Get order details
     */
    Order GetOrder(const std::string& inst_id,
                   const std::string& order_id = "",
                   const std::string& client_order_id = "");
    
    /**
     * @brief Get pending orders
     */
    std::vector<Order> GetPendingOrders(const std::string& inst_id = "");
    
    /**
     * @brief Get order history (last 7 days)
     */
    std::vector<Order> GetOrderHistory(const std::string& inst_id = "",
                                       const std::string& state = "",
                                       uint64_t begin_time = 0,
                                       uint64_t end_time = 0,
                                       int limit = 100);
    
    /**
     * @brief Get order archive (last 3 months)
     */
    std::vector<Order> GetOrderArchive(const std::string& inst_id = "",
                                       uint64_t begin_time = 0,
                                       uint64_t end_time = 0,
                                       int limit = 100);
    
    // ==================== Trade History ====================
    
    /**
     * @brief Get fills (trade execution records)
     */
    struct Fill {
        std::string inst_id;
        std::string order_id;
        std::string trade_id;
        std::string fill_id;
        std::string side;  // buy, sell
        double fill_price;
        double fill_size;
        double fee;
        std::string fee_currency;
        uint64_t fill_time;
        std::string exec_type;  // T: taker, M: maker
    };
    std::vector<Fill> GetFills(const std::string& inst_id = "",
                               uint64_t begin_time = 0,
                               uint64_t end_time = 0,
                               int limit = 100);
    
    // ==================== Bills (Account Ledger) ====================
    
    /**
     * @brief Get bills (account ledger)
     */
    struct Bill {
        std::string bill_id;
        std::string inst_id;
        std::string currency;
        int bill_type;  // 1: Transfer, 2: Trade, 5: Funding fee, etc.
        std::string bill_sub_type;
        double balance_change;
        double balance;
        double fee;
        uint64_t timestamp;
        std::string notes;
    };
    std::vector<Bill> GetBills(const std::string& inst_id = "",
                               int bill_type = -1,
                               uint64_t begin_time = 0,
                               uint64_t end_time = 0,
                               int limit = 100);
    
    // ==================== Utility ====================
    
    /**
     * @brief Check if API is connected and authenticated
     */
    bool TestConnection();
    
    /**
     * @brief Get API statistics
     */
    struct APIStatistics {
        uint64_t total_requests;
        uint64_t successful_requests;
        uint64_t failed_requests;
        double avg_response_time_ms;
        double success_rate;
    };
    APIStatistics GetStatistics() const;
    
private:
    // Helper functions
    json MakeRequest(const std::string& method,
                    const std::string& endpoint,
                    const json& params = json::object(),
                    bool is_private = false);
    
    std::map<std::string, std::string> GetAuthHeaders(const std::string& method,
                                                       const std::string& request_path,
                                                       const std::string& body);
    
    // Parse response helpers
    Tick ParseTicker(const json& data);
    Depth ParseOrderBook(const json& data);
    Order ParseOrder(const json& data);
    Position ParsePosition(const json& data);
    Account ParseAccount(const json& data);
    
private:
    std::unique_ptr<HttpClient> http_client_;
    std::unique_ptr<OKXSigner> signer_;
    APIConfig config_;
    bool initialized_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    APIStatistics stats_;
};

#endif // OKX_REST_API_H
