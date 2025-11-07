#include "okx_rest_api.h"
#include "config.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>


// Color codes for terminal output
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"

// Helper function to find config file
std::string FindConfigFile() {
    // Try multiple possible paths
    std::vector<std::string> paths = {
        "config/config.json",           // From project root
        "../config/config.json",        // From build directory
        "./config/config.json",         // Current directory
        "../../config/config.json"      // Extra level up
    };
    
    for (const auto& path : paths) {
        std::ifstream f(path);
        if (f.good()) {
            std::cout << "Found config file at: " << path << std::endl;
            return path;
        }
    }
    
    return "";
}

class APIValidator {
public:
    APIValidator(const std::string& config_path) {
        // Load configuration
        if (!Config::Instance().Load(config_path)) {
            throw std::runtime_error("Failed to load config file: " + config_path);
        }
        
        // Initialize API
        OKXRestAPI::APIConfig config;
        config.base_url = Config::Instance().GetOKXRESTURL();
        config.api_key = Config::Instance().GetOKXAPIKey();
        config.secret_key = Config::Instance().GetOKXSecretKey();
        config.passphrase = Config::Instance().GetOKXPassphrase();
        config.is_simulation = Config::Instance().IsSimulation();
        
        api_.Initialize(config);
    }
    
    void RunAllTests() {
        PrintHeader("OKX API Complete Validation Tool");
        
        // Test connection first
        std::cout << "\n" << CYAN << BOLD << "=== Testing Connection ===" << RESET << "\n";
        if (TestConnection()) {
            std::cout << GREEN << "✓ Connection successful" << RESET << "\n";
        } else {
            std::cout << RED << "✗ Connection failed" << RESET << "\n";
            return;
        }
        
        // Run all tests
        TestPublicAPI();
        TestPrivateAPI();
        TestTradingAPI();
        TestHistoryAPI();
        
        // Print statistics
        PrintStatistics();
    }
    
private:
    bool TestConnection() {
        return api_.TestConnection();
    }
    
    void TestPublicAPI() {
        PrintSection("Public Market Data API");
        
        std::string inst_id = "XAUT-USDT-SWAP";
        
        // Test 1: Get Ticker
        std::cout << "\n" << YELLOW << "[Test 1] Get Ticker for " << inst_id << RESET << "\n";
        Tick tick = api_.GetTicker(inst_id);
        PrintTick(tick);
        
        // Test 2: Get Order Book
        std::cout << "\n" << YELLOW << "[Test 2] Get Order Book (5 levels)" << RESET << "\n";
        Depth depth = api_.GetOrderBook(inst_id, 5);
        PrintDepth(depth);
        
        // Test 3: Get Funding Rate
        std::cout << "\n" << YELLOW << "[Test 3] Get Funding Rate" << RESET << "\n";
        auto funding = api_.GetFundingRate(inst_id);
        PrintFundingRate(funding);
        
        // Test 4: Get Candlesticks
        std::cout << "\n" << YELLOW << "[Test 4] Get Candlesticks (last 5 bars)" << RESET << "\n";
        auto candles = api_.GetCandlesticks(inst_id, "1H", 5);
        PrintCandlesticks(candles);
        
        // Test 5: Get Instrument Info
        std::cout << "\n" << YELLOW << "[Test 5] Get Instrument Info" << RESET << "\n";
        // auto info = api_.GetInstrumentInfo(inst_id);
        // PrintInstrumentInfo(info);
            std::cout << "[Test 5] SKIPPED\n";
    }
    
    void TestPrivateAPI() {
        PrintSection("Private Account API");
        
        // Test 6: Get Account Balance
        std::cout << "\n" << YELLOW << "[Test 6] Get Account Balance" << RESET << "\n";
        Account account = api_.GetAccountBalance();
        PrintAccount(account);
        
        // Test 7: Get Positions
        std::cout << "\n" << YELLOW << "[Test 7] Get All Positions" << RESET << "\n";
        auto positions = api_.GetPositions();
        PrintPositions(positions);
        
        // Test 8: Get Account Configuration
        std::cout << "\n" << YELLOW << "[Test 8] Get Account Configuration" << RESET << "\n";
        auto config = api_.GetAccountConfig();
        PrintAccountConfig(config);
    }
    
    void TestTradingAPI() {
        PrintSection("Trading API");
        
        // Test 9: Get Pending Orders
        std::cout << "\n" << YELLOW << "[Test 9] Get Pending Orders" << RESET << "\n";
        auto orders = api_.GetPendingOrders();
        PrintOrders(orders);
        
        // Test 10: Get Order History
        std::cout << "\n" << YELLOW << "[Test 10] Get Order History (last 10)" << RESET << "\n";
        auto history = api_.GetOrderHistory("", "", 0, 0, 10);
        PrintOrders(history);
    }
    
    void TestHistoryAPI() {
        PrintSection("Trade History API");
        
        // Test 11: Get Fills
        std::cout << "\n" << YELLOW << "[Test 11] Get Fills (last 10)" << RESET << "\n";
        auto fills = api_.GetFills("", 0, 0, 10);
        PrintFills(fills);
        
        // Test 12: Get Bills
        std::cout << "\n" << YELLOW << "[Test 12] Get Bills (last 10)" << RESET << "\n";
        auto bills = api_.GetBills("", -1, 0, 0, 10);
        PrintBills(bills);
    }
    
    // ==================== Print Functions ====================
    
    void PrintHeader(const std::string& title) {
        std::cout << "\n" << BOLD << CYAN;
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║" << std::setw(64) << std::left << ("  " + title) << "║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝";
        std::cout << RESET << "\n";
    }
    
    void PrintSection(const std::string& title) {
        std::cout << "\n\n" << BOLD << MAGENTA;
        std::cout << "┌────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ " << std::setw(62) << std::left << title << "│\n";
        std::cout << "└────────────────────────────────────────────────────────────────┘";
        std::cout << RESET << "\n";
    }
    
    void PrintTick(const Tick& tick) {
        std::cout << "Instrument:    " << BOLD << tick.inst_id << RESET << "\n";
        std::cout << "Last Price:    " << GREEN << tick.last_price << RESET << "\n";
        std::cout << "Bid:           " << tick.bid_price << " (" << tick.bid_size << ")\n";
        std::cout << "Ask:           " << tick.ask_price << " (" << tick.ask_size << ")\n";
        std::cout << "Spread:        " << YELLOW << (tick.ask_price - tick.bid_price) << RESET << "\n";
        std::cout << "24h High:      " << tick.high_24h << "\n";
        std::cout << "24h Low:       " << tick.low_24h << "\n";
        std::cout << "24h Volume:    " << tick.volume_24h << "\n";
        std::cout << "Timestamp:     " << FormatTimestamp(tick.timestamp) << "\n";
    }
    
    void PrintDepth(const Depth& depth) {
        std::cout << "\n" << BOLD << "Order Book:" << RESET << "\n";
        std::cout << "┌─────────────┬─────────────┬─────────────┬─────────────┐\n";
        std::cout << "│ " << BOLD << "Bid Size" << RESET;
        std::cout << "    │ " << BOLD << "Bid Price" << RESET;
        std::cout << "   │ " << BOLD << "Ask Price" << RESET;
        std::cout << "   │ " << BOLD << "Ask Size" << RESET << "    │\n";
        std::cout << "├─────────────┼─────────────┼─────────────┼─────────────┤\n";
        
        size_t max_levels = std::min(depth.bids.size(), depth.asks.size());
        for (size_t i = 0; i < max_levels; i++) {
            std::cout << "│ " << std::setw(11) << std::fixed << std::setprecision(2) 
                     << depth.bids[i].size << " │ "
                     << GREEN << std::setw(11) << std::fixed << std::setprecision(2) 
                     << depth.bids[i].price << RESET << " │ "
                     << RED << std::setw(11) << std::fixed << std::setprecision(2) 
                     << depth.asks[i].price << RESET << " │ "
                     << std::setw(11) << std::fixed << std::setprecision(2) 
                     << depth.asks[i].size << " │\n";
        }
        std::cout << "└─────────────┴─────────────┴─────────────┴─────────────┘\n";
        
        // Test average price calculation
        double avg_buy_price = depth.CalculateAvgPrice("buy", 10.0);
        double avg_sell_price = depth.CalculateAvgPrice("sell", 10.0);
        std::cout << "\nAverage Price (10 units):\n";
        std::cout << "  Buy:  " << GREEN << avg_buy_price << RESET << "\n";
        std::cout << "  Sell: " << RED << avg_sell_price << RESET << "\n";
    }
    
    void PrintFundingRate(const OKXRestAPI::FundingRate& rate) {
        std::cout << "Instrument:         " << rate.inst_id << "\n";
        std::cout << "Current Rate:       " << rate.funding_rate << " (" 
                 << (rate.funding_rate * 100) << "%)\n";
        std::cout << "Funding Time:       " << FormatTimestamp(rate.funding_time) << "\n";
        std::cout << "Next Rate:          " << rate.next_funding_rate << " (" 
                 << (rate.next_funding_rate * 100) << "%)\n";
        std::cout << "Next Funding Time:  " << FormatTimestamp(rate.next_funding_time) << "\n";
    }
    
    void PrintCandlesticks(const std::vector<OKXRestAPI::Candlestick>& candles) {
        std::cout << "┌──────────────────────┬────────┬────────┬────────┬────────┬─────────┐\n";
        std::cout << "│ " << BOLD << "Timestamp" << RESET;
        std::cout << "            │ " << BOLD << "Open" << RESET;
        std::cout << "   │ " << BOLD << "High" << RESET;
        std::cout << "   │ " << BOLD << "Low" << RESET;
        std::cout << "    │ " << BOLD << "Close" << RESET;
        std::cout << "  │ " << BOLD << "Volume" << RESET << "  │\n";
        std::cout << "├──────────────────────┼────────┼────────┼────────┼────────┼─────────┤\n";
        
        for (const auto& candle : candles) {
            std::cout << "│ " << FormatTimestamp(candle.timestamp) << " │ "
                     << std::setw(6) << std::fixed << std::setprecision(2) << candle.open << " │ "
                     << std::setw(6) << candle.high << " │ "
                     << std::setw(6) << candle.low << " │ "
                     << std::setw(6) << candle.close << " │ "
                     << std::setw(7) << std::setprecision(0) << candle.volume << " │\n";
        }
        std::cout << "└──────────────────────┴────────┴────────┴────────┴────────┴─────────┘\n";
    }
    
    void PrintInstrumentInfo(const OKXRestAPI::InstrumentInfo& info) {
        std::cout << "Instrument ID:   " << BOLD << info.inst_id << RESET << "\n";
        std::cout << "Type:            " << info.inst_type << "\n";
        std::cout << "Underlying:      " << info.underlying << "\n";
        std::cout << "Base Currency:   " << info.base_ccy << "\n";
        std::cout << "Quote Currency:  " << info.quote_ccy << "\n";
        std::cout << "Settle Currency: " << info.settle_ccy << "\n";
        std::cout << "Contract Value:  " << info.contract_val << "\n";
        std::cout << "Tick Size:       " << info.tick_size << "\n";
        std::cout << "Lot Size:        " << info.lot_size << "\n";
        std::cout << "Min Size:        " << info.min_size << "\n";
        std::cout << "State:           " << (info.state == "live" ? GREEN : YELLOW) 
                 << info.state << RESET << "\n";
    }
    
    void PrintAccount(const Account& account) {
        std::cout << "Total Equity:           " << BOLD << account.total_equity << RESET << " USDT\n";
        std::cout << "Adjusted Equity:        " << account.adj_equity << " USDT\n";
        std::cout << "Margin Ratio:           " << account.margin_ratio << "\n";
        std::cout << "Maintenance Margin:     " << account.maintenance_margin_ratio << "\n";
        std::cout << "Last Update:            " << FormatTimestamp(account.update_time) << "\n";
        
        if (!account.details.empty()) {
            std::cout << "\n" << BOLD << "Currency Details:" << RESET << "\n";
            std::cout << "┌──────────┬──────────────┬──────────────┬──────────────┐\n";
            std::cout << "│ Currency │ Equity       │ Available    │ Frozen       │\n";
            std::cout << "├──────────┼──────────────┼──────────────┼──────────────┤\n";
            
            for (const auto& detail : account.details) {
                std::cout << "│ " << std::setw(8) << std::left << detail.currency << " │ "
                         << std::setw(12) << std::fixed << std::setprecision(4) << detail.equity << " │ "
                         << std::setw(12) << detail.available_balance << " │ "
                         << std::setw(12) << detail.frozen_balance << " │\n";
            }
            std::cout << "└──────────┴──────────────┴──────────────┴──────────────┘\n";
        }
    }
    
    void PrintPositions(const std::vector<Position>& positions) {
        if (positions.empty()) {
            std::cout << "No open positions\n";
            return;
        }
        
        std::cout << "Total Positions: " << BOLD << positions.size() << RESET << "\n\n";
        
        for (const auto& pos : positions) {
            std::cout << "─────────────────────────────────────\n";
            std::cout << "Instrument:     " << BOLD << pos.inst_id << RESET << "\n";
            std::cout << "Side:           " << (pos.position_side == "long" ? GREEN : RED) 
                     << pos.position_side << RESET << "\n";
            std::cout << "Position:       " << pos.position << "\n";
            std::cout << "Avg Price:      " << pos.avg_price << "\n";
            std::cout << "Mark Price:     " << pos.mark_price << "\n";
            std::cout << "Liq Price:      " << pos.liquidation_price << "\n";
            std::cout << "Unrealized PnL: " << (pos.unrealized_pnl >= 0 ? GREEN : RED) 
                     << pos.unrealized_pnl << RESET << " (" 
                     << (pos.unrealized_pnl_ratio * 100) << "%)\n";
            std::cout << "Leverage:       " << pos.leverage << "x\n";
            std::cout << "Margin:         " << pos.margin << "\n";
            std::cout << "Margin Ratio:   " << pos.margin_ratio << "\n";
        }
        std::cout << "─────────────────────────────────────\n";
    }
    
    void PrintAccountConfig(const OKXRestAPI::AccountConfig& config) {
        std::cout << "Position Mode:  " << (config.position_mode == 1 ? "Long/Short" : "Net") << "\n";
        std::cout << "Auto Loan:      " << (config.auto_loan ? "Enabled" : "Disabled") << "\n";
        std::cout << "Account Level:  " << config.account_level << "\n";
    }
    
    void PrintOrders(const std::vector<Order>& orders) {
        if (orders.empty()) {
            std::cout << "No orders found\n";
            return;
        }
        
        std::cout << "Total Orders: " << BOLD << orders.size() << RESET << "\n\n";
        
        for (size_t i = 0; i < std::min(orders.size(), size_t(5)); i++) {
            const auto& order = orders[i];
            std::cout << "─────────────────────────────────────\n";
            std::cout << "Order ID:       " << order.order_id << "\n";
            std::cout << "Instrument:     " << order.inst_id << "\n";
            std::cout << "Side:           " << (order.side == "buy" ? GREEN : RED) 
                     << order.side << RESET << "\n";
            std::cout << "Type:           " << order.order_type << "\n";
            std::cout << "Price:          " << order.price << "\n";
            std::cout << "Size:           " << order.size << "\n";
            std::cout << "Filled:         " << order.filled_size << "\n";
            std::cout << "State:          " << order.state << "\n";
            std::cout << "Create Time:    " << FormatTimestamp(order.create_time) << "\n";
        }
        
        if (orders.size() > 5) {
            std::cout << "... and " << (orders.size() - 5) << " more orders\n";
        }
        std::cout << "─────────────────────────────────────\n";
    }
    
    void PrintFills(const std::vector<OKXRestAPI::Fill>& fills) {
        if (fills.empty()) {
            std::cout << "No fills found\n";
            return;
        }
        
        std::cout << "Total Fills: " << BOLD << fills.size() << RESET << "\n\n";
        std::cout << "┌──────────────┬──────┬────────┬─────────┬──────────┬────────┐\n";
        std::cout << "│ Order ID     │ Side │ Price  │ Size    │ Fee      │ Type   │\n";
        std::cout << "├──────────────┼──────┼────────┼─────────┼──────────┼────────┤\n";
        
        for (size_t i = 0; i < std::min(fills.size(), size_t(10)); i++) {
            const auto& fill = fills[i];
            std::cout << "│ " << std::setw(12) << fill.order_id.substr(0, 12) << " │ "
                     << (fill.side == "buy" ? GREEN : RED) << std::setw(4) << fill.side << RESET << " │ "
                     << std::setw(6) << std::fixed << std::setprecision(2) << fill.fill_price << " │ "
                     << std::setw(7) << std::setprecision(4) << fill.fill_size << " │ "
                     << std::setw(8) << std::setprecision(4) << fill.fee << " │ "
                     << std::setw(6) << fill.exec_type << " │\n";
        }
        std::cout << "└──────────────┴──────┴────────┴─────────┴──────────┴────────┘\n";
    }
    
    void PrintBills(const std::vector<OKXRestAPI::Bill>& bills) {
        if (bills.empty()) {
            std::cout << "No bills found\n";
            return;
        }
        
        std::cout << "Total Bills: " << BOLD << bills.size() << RESET << "\n\n";
        std::cout << "┌──────────────┬──────┬────────┬─────────────┬─────────────┐\n";
        std::cout << "│ Bill ID      │ Type │ Ccy    │ Change      │ Balance     │\n";
        std::cout << "├──────────────┼──────┼────────┼─────────────┼─────────────┤\n";
        
        for (size_t i = 0; i < std::min(bills.size(), size_t(10)); i++) {
            const auto& bill = bills[i];
            std::cout << "│ " << std::setw(12) << bill.bill_id.substr(0, 12) << " │ "
                     << std::setw(4) << bill.bill_type << " │ "
                     << std::setw(6) << bill.currency << " │ "
                     << (bill.balance_change >= 0 ? GREEN : RED) << std::setw(11) 
                     << std::fixed << std::setprecision(4) << bill.balance_change << RESET << " │ "
                     << std::setw(11) << std::setprecision(4) << bill.balance << " │\n";
        }
        std::cout << "└──────────────┴──────┴────────┴─────────────┴─────────────┘\n";
    }
    
    void PrintStatistics() {
        PrintSection("API Statistics");
        
        auto stats = api_.GetStatistics();
        
        std::cout << "Total Requests:      " << stats.total_requests << "\n";
        std::cout << "Successful:          " << GREEN << stats.successful_requests << RESET << "\n";
        std::cout << "Failed:              " << RED << stats.failed_requests << RESET << "\n";
        std::cout << "Success Rate:        " << (stats.success_rate) << "%\n";
        std::cout << "Avg Response Time:   " << stats.avg_response_time_ms << " ms\n";
    }
    
    // Helper functions
    std::string FormatTimestamp(uint64_t timestamp) {
        time_t t = timestamp / 1000;
        struct tm tm;
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        return buffer;
    }
    
private:
    OKXRestAPI api_;
};

int main(int argc, char* argv[]) {
// 设置控制台为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    try {
        std::string config_path;
        
        // Try to get config path from command line or auto-find
        if (argc > 1) {
            config_path = argv[1];
            std::cout << "Using config file: " << config_path << std::endl;
        } else {
            std::cout << "No config file specified, searching..." << std::endl;
            config_path = FindConfigFile();
            if (config_path.empty()) {
                std::cerr << RED << "Error: Cannot find config.json" << RESET << std::endl;
                std::cerr << "Tried paths:" << std::endl;
                std::cerr << "  - config/config.json" << std::endl;
                std::cerr << "  - ../config/config.json" << std::endl;
                std::cerr << "  - ./config/config.json" << std::endl;
                std::cerr << "\nUsage: " << argv[0] << " [config_file_path]" << std::endl;
                return 1;
            }
        }
        
        // Run validation
        APIValidator validator(config_path);
        validator.RunAllTests();
        
        std::cout << "\n" << GREEN << BOLD << "All tests completed!" << RESET << "\n\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << RED << "Error: " << e.what() << RESET << std::endl;
        return 1;
    }
}
