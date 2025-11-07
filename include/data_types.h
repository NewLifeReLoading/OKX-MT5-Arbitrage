#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <mutex>

// ============================================================================
// Basic Type Definitions
// ============================================================================

using Timestamp = uint64_t;  // Millisecond timestamp
using Price = double;
using Volume = double;

// ============================================================================
// Market Data
// ============================================================================

/**
 * @brief Tick data - Real-time quote
 */
struct Tick {
    // Basic fields (compatible with Step 1)
    std::string symbol;         // Trading pair (for compatibility)
    std::string platform;       // Platform (okx/mt5)
    
    Price bid;                  // Bid price (compatibility)
    Price ask;                  // Ask price (compatibility)
    Price last;                 // Last price (compatibility)
    
    Volume bid_size;            // Bid size
    Volume ask_size;            // Ask size
    
    Timestamp timestamp;        // Timestamp
    
    Price mark_price;           // Mark price
    double funding_rate;        // Funding rate
    
    // OKX complete fields (for API)
    std::string inst_id;        // Instrument ID (OKX format)
    double last_price;          // Last price
    double bid_price;           // Bid price
    double ask_price;           // Ask price
    double high_24h;            // 24h high
    double low_24h;             // 24h low
    double volume_24h;          // 24h volume
    double volume_currency_24h; // 24h volume in currency
    
    Tick() : bid(0), ask(0), last(0), bid_size(0), ask_size(0),
             timestamp(0), mark_price(0), funding_rate(0),
             last_price(0), bid_price(0), ask_price(0),
             high_24h(0), low_24h(0), volume_24h(0), volume_currency_24h(0) {}
};

/**
 * @brief Depth level
 */
struct DepthLevel {
    Price price;      // Price
    Volume size;      // Size
    
    DepthLevel() : price(0), size(0) {}
    DepthLevel(Price p, Volume s) : price(p), size(s) {}
};

/**
 * @brief Depth data - Order book
 */
struct Depth {
    // Basic fields (compatible with Step 1)
    std::string symbol;         // Trading pair (for compatibility)
    std::string platform;       // Platform
    
    std::vector<DepthLevel> bids;  // Bids (price high to low)
    std::vector<DepthLevel> asks;  // Asks (price low to high)
    
    Timestamp timestamp;        // Timestamp
    
    // OKX complete fields
    std::string inst_id;        // Instrument ID (OKX format)
    
    Depth() : timestamp(0) {}
    
    /**
     * @brief Calculate average price for a given size
     */
    Price CalculateAvgPrice(const std::string& side, Volume target_size) const {
        const auto& levels = (side == "buy") ? asks : bids;
        
        double total_cost = 0;
        double filled_size = 0;
        
        for (const auto& level : levels) {
            if (filled_size >= target_size) break;
            
            double fill = std::min(target_size - filled_size, level.size);
            total_cost += fill * level.price;
            filled_size += fill;
        }
        
        if (filled_size < target_size) return 0;  // Not enough depth
        return total_cost / filled_size;
    }
};

// ============================================================================
// Order Data
// ============================================================================

/**
 * @brief Order (with complete OKX fields)
 */
struct Order {
    // Basic fields (compatible with Step 1)
    std::string order_id;       // Order ID
    std::string symbol;         // Trading pair (for compatibility)
    std::string platform;       // Platform
    std::string side;           // buy/sell
    std::string order_type;     // market/limit
    std::string state;          // live/filled/canceled
    
    Price price;                // Order price
    Volume size;                // Order size
    Volume filled_size;         // Filled size
    Price avg_price;            // Average fill price (compatibility)
    
    double fee;                 // Fee
    double pnl;                 // PnL (self-calculated)
    
    Timestamp create_time;      // Create time
    Timestamp update_time;      // Update time
    
    // User-defined (Step 1)
    int group_id;               // Group ID (for hedge orders)
    std::string notes;          // Notes
    
    // OKX complete fields
    std::string client_order_id;  // Client order ID
    std::string inst_id;          // Instrument ID
    std::string position_side;    // Position side (long/short/net)
    std::string trade_mode;       // Trade mode (cross/isolated)
    double avg_fill_price;        // Average fill price (OKX field)
    int leverage;                 // Leverage
    double tp_trigger_price;      // Take profit trigger price
    double tp_order_price;        // Take profit order price
    double sl_trigger_price;      // Stop loss trigger price
    double sl_order_price;        // Stop loss order price
    
    Order() : price(0), size(0), filled_size(0), avg_price(0),
              fee(0), pnl(0), create_time(0), update_time(0), group_id(0),
              avg_fill_price(0), leverage(1),
              tp_trigger_price(0), tp_order_price(0),
              sl_trigger_price(0), sl_order_price(0) {}
};

/**
 * @brief Position (with complete OKX fields)
 */
struct Position {
    // Basic fields (compatible with Step 1)
    std::string symbol;         // Trading pair (for compatibility)
    std::string platform;       // Platform
    std::string side;           // long/short (compatibility)
    
    Volume size;                // Position size (compatibility)
    Price avg_price;            // Average price
    Price mark_price;           // Mark price
    
    double unrealized_pnl;      // Unrealized PnL (self-calculated)
    double margin;              // Margin
    
    Timestamp update_time;      // Update time
    
    // OKX complete fields
    std::string inst_id;          // Instrument ID
    std::string position_side;    // Position side (long/short/net)
    double position;              // Position size (OKX field)
    double available_position;    // Available position
    double liquidation_price;     // Liquidation price
    double unrealized_pnl_ratio;  // Unrealized PnL ratio
    int leverage;                 // Leverage
    double margin_ratio;          // Margin ratio
    double initial_margin;        // Initial margin
    double maintenance_margin;    // Maintenance margin
    std::string trade_mode;       // Trade mode
    Timestamp create_time;        // Create time
    
    Position() : size(0), avg_price(0), mark_price(0),
                 unrealized_pnl(0), margin(0), update_time(0),
                 position(0), available_position(0), liquidation_price(0),
                 unrealized_pnl_ratio(0), leverage(1), margin_ratio(0),
                 initial_margin(0), maintenance_margin(0), create_time(0) {}
};

/**
 * @brief Account (with complete OKX fields)
 */
struct Account {
    // Basic fields (compatible with Step 1)
    std::string platform;       // Platform
    
    double balance;             // Balance (compatibility)
    double equity;              // Equity (compatibility)
    double margin;              // Used margin
    double free_margin;         // Free margin
    double unrealized_pnl;      // Unrealized PnL
    
    Timestamp update_time;      // Update time
    
    // OKX complete fields
    double total_equity;              // Total equity
    double isolated_equity;           // Isolated equity
    double adj_equity;                // Adjusted equity
    double margin_ratio;              // Margin ratio
    double maintenance_margin_ratio;  // Maintenance margin ratio
    double initial_margin_ratio;      // Initial margin ratio
    
    // Currency details
    struct Detail {
        std::string currency;           // Currency
        double equity;                  // Equity
        double cash_balance;            // Cash balance
        double available_balance;       // Available balance
        double frozen_balance;          // Frozen balance
        double order_frozen;            // Order frozen
        double available_equity;        // Available equity
        double unrealized_pnl;          // Unrealized PnL
        
        Detail() : equity(0), cash_balance(0), available_balance(0),
                   frozen_balance(0), order_frozen(0), available_equity(0),
                   unrealized_pnl(0) {}
    };
    
    std::vector<Detail> details;  // Currency details
    
    Account() : balance(0), equity(0), margin(0),
                free_margin(0), unrealized_pnl(0), update_time(0),
                total_equity(0), isolated_equity(0), adj_equity(0),
                margin_ratio(0), maintenance_margin_ratio(0),
                initial_margin_ratio(0) {}
};

// ============================================================================
// Strategy Related
// ============================================================================

/**
 * @brief Strategy parameters
 */
struct StrategyParams {
    double first_order;         // First order spread
    double next_order;          // Next order spread
    int max_orders;             // Max open orders
    double take_profit;         // Take profit
    
    double okx_fee_rate;        // OKX fee rate
    double mt5_fee_rate;        // MT5 fee rate
    
    double okx_order_size;      // OKX order size
    double mt5_order_size;      // MT5 order size
    
    StrategyParams() : first_order(0), next_order(0), max_orders(0),
                       take_profit(0), okx_fee_rate(0), mt5_fee_rate(0),
                       okx_order_size(0), mt5_order_size(0) {}
};

/**
 * @brief Order group (pair of hedge orders)
 */
struct OrderGroup {
    int group_id;               // Group ID
    
    Order okx_order;            // OKX order
    Order mt5_order;            // MT5 order
    
    double total_pnl;           // Total PnL
    double total_fee;           // Total fee
    
    std::string status;         // Status: open/closed
    
    Timestamp create_time;      // Create time
    Timestamp close_time;       // Close time
    
    OrderGroup() : group_id(0), total_pnl(0), total_fee(0),
                   create_time(0), close_time(0) {}
};

#endif // DATA_TYPES_H
