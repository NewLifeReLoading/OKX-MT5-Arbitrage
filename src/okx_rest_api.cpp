#include "okx_rest_api.h"
#include <iostream>
#include <sstream>

// 辅助函数：从 inst_id 推断 instType
// 在文件开头，GetInstType() 函数之后添加：
namespace {
    std::string GetInstType(const std::string& inst_id) {
        if (inst_id.find("-SWAP") != std::string::npos) {
            return "SWAP";
        } else if (inst_id.find("-FUTURES-") != std::string::npos) {
            return "FUTURES";
        } else if (inst_id.find("-OPTION-") != std::string::npos) {
            return "OPTION";
        } else {
            return "SPOT";
        }
    }

    // 添加安全的字符串转换函数
    double SafeStod(const std::string& str, double default_value = 0.0) {
        if (str.empty()) {
            return default_value;
        }
        try {
            return std::stod(str);
        } catch (...) {
            return default_value;
        }
    }

    uint64_t SafeStoull(const std::string& str, uint64_t default_value = 0) {
        if (str.empty()) {
            return default_value;
        }
        try {
            return std::stoull(str);
        } catch (...) {
            return default_value;
        }
    }

    int SafeStoi(const std::string& str, int default_value = 0) {
        if (str.empty()) {
            return default_value;
        }
        try {
            return SafeStoi(str);
        } catch (...) {
            return default_value;
        }
    }
}


OKXRestAPI::OKXRestAPI()
    : initialized_(false) {
}

OKXRestAPI::~OKXRestAPI() {
}

bool OKXRestAPI::Initialize(const APIConfig& config) {
    config_ = config;

    // Initialize HTTP client
    http_client_ = std::make_unique<HttpClient>();
    HttpClient::RequestOptions options;
    options.timeout_ms = config.timeout_ms;
    options.max_retries = config.max_retries;
    options.max_requests_per_second = 10;  // OKX rate limit

    if (!http_client_->Initialize(options)) {
        std::cerr << "Failed to initialize HTTP client" << std::endl;
        return false;
    }

    // Set default headers
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Accept"] = "application/json";
    http_client_->SetDefaultHeaders(headers);

    // Initialize signer
    if (!config.api_key.empty()) {
        signer_ = std::make_unique<OKXSigner>(
            config.api_key,
            config.secret_key,
            config.passphrase
        );
    }

    initialized_ = true;
    return true;
}

// ==================== Public Market Data ====================

Tick OKXRestAPI::GetTicker(const std::string& inst_id) {
    json params = {
        {"instId", inst_id}  // ← 删除了instType那行
    };

    json response = MakeRequest("GET", "/api/v5/market/ticker", params, false);

    Tick tick;
    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        tick = ParseTicker(response["data"][0]);
    }

    return tick;
}

Depth OKXRestAPI::GetOrderBook(const std::string& inst_id, int depth_size) {
    json params = {
        {"instId", inst_id},
        {"sz", std::to_string(depth_size)}  // ← 删除了instType那行
    };

    json response = MakeRequest("GET", "/api/v5/market/books", params, false);

    Depth depth;
    depth.inst_id = inst_id;
    depth.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        depth = ParseOrderBook(response["data"][0]);
        depth.inst_id = inst_id;
    }

    return depth;
}

OKXRestAPI::FundingRate OKXRestAPI::GetFundingRate(const std::string& inst_id) {
    json params = {
        {"instId", inst_id}  // ← 删除了instType那行
    };

    json response = MakeRequest("GET", "/api/v5/public/funding-rate", params, false);

    FundingRate rate;
    rate.inst_id = inst_id;

    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        const auto& data = response["data"][0];
        rate.funding_rate = std::stod(data.value("fundingRate", "0"));
        rate.funding_time = std::stod(data.value("fundingTime", "0"));
        rate.next_funding_rate = std::stod(data.value("nextFundingRate", "0"));
        rate.next_funding_time = std::stod(data.value("nextFundingTime", "0"));
    }

    return rate;
}

std::vector<OKXRestAPI::Candlestick> OKXRestAPI::GetCandlesticks(
    const std::string& inst_id,
    const std::string& bar,
    int limit) {

    json params = {
        {"instId", inst_id},
        {"bar", bar},                         // ← 删除了instType那行
        {"limit", std::to_string(limit)}
    };

    json response = MakeRequest("GET", "/api/v5/market/candles", params, false);

    std::vector<Candlestick> candles;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            if (item.is_array() && item.size() >= 7) {
                Candlestick candle;
                candle.timestamp = std::stoull(item[0].get<std::string>());
                candle.open = std::stod(item[1].get<std::string>());
                candle.high = std::stod(item[2].get<std::string>());
                candle.low = std::stod(item[3].get<std::string>());
                candle.close = std::stod(item[4].get<std::string>());
                candle.volume = std::stod(item[5].get<std::string>());
                candle.volume_currency = std::stod(item[6].get<std::string>());
                candles.push_back(candle);
            }
        }
    }

    return candles;
}

OKXRestAPI::InstrumentInfo OKXRestAPI::GetInstrumentInfo(const std::string& inst_id) {
    json params = {
        {"instId", inst_id},
        {"instType", GetInstType(inst_id)}
    };

    json response = MakeRequest("GET", "/api/v5/public/instruments", params, false);

    InstrumentInfo info;
    info.inst_id = inst_id;

    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        const auto& data = response["data"][0];
        info.inst_type = data.value("instType", "");
        info.underlying = data.value("uly", "");
        info.base_ccy = data.value("baseCcy", "");
        info.quote_ccy = data.value("quoteCcy", "");
        info.settle_ccy = data.value("settleCcy", "");
        info.contract_val = std::stod(data.value("ctVal", "0"));
        info.tick_size = std::stod(data.value("tickSz", "0"));
        info.lot_size = std::stod(data.value("lotSz", "0"));
        info.min_size = std::stod(data.value("minSz", "0"));
        info.state = data.value("state", "");
    }

    return info;
}

// ==================== Account API ====================

Account OKXRestAPI::GetAccountBalance() {
    json response = MakeRequest("GET", "/api/v5/account/balance", json::object(), true);

    Account account;

    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        account = ParseAccount(response["data"][0]);
    }

    return account;
}

std::vector<Position> OKXRestAPI::GetPositions(const std::string& inst_id) {
    json params = json::object();
    if (!inst_id.empty()) {
        params["instId"] = inst_id;
    }

    json response = MakeRequest("GET", "/api/v5/account/positions", params, true);

    std::vector<Position> positions;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            positions.push_back(ParsePosition(item));
        }
    }

    return positions;
}

OKXRestAPI::AccountConfig OKXRestAPI::GetAccountConfig() {
    json response = MakeRequest("GET", "/api/v5/account/config", json::object(), true);

    AccountConfig config;

    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        const auto& data = response["data"][0];
        config.position_mode = SafeStoi(data.value("posMode", "0"));
        config.auto_loan = data.value("autoLoan", false);
        config.level = SafeStoi(data.value("level", "0"));
        config.account_level = data.value("acctLv", "");
    }

    return config;
}

bool OKXRestAPI::SetLeverage(const std::string& inst_id,
                             int leverage,
                             const std::string& margin_mode) {
    json body = {
        {"instId", inst_id},
        {"lever", std::to_string(leverage)},
        {"mgnMode", margin_mode}
    };

    json response = MakeRequest("POST", "/api/v5/account/set-leverage", body, true);

    return !response.empty() && response.value("code", "") == "0";
}

// ==================== Trading API ====================

std::string OKXRestAPI::PlaceOrder(const Order& order) {
    json body = {
        {"instId", order.inst_id},
        {"tdMode", order.trade_mode},
        {"side", order.side},
        {"ordType", order.order_type},
        {"sz", std::to_string(order.size)}
    };

    // Optional fields
    if (!order.position_side.empty()) {
        body["posSide"] = order.position_side;
    }
    if (order.price > 0) {
        body["px"] = std::to_string(order.price);
    }
    if (!order.client_order_id.empty()) {
        body["clOrdId"] = order.client_order_id;
    }
    if (order.tp_trigger_price > 0) {
        body["tpTriggerPx"] = std::to_string(order.tp_trigger_price);
        body["tpOrdPx"] = std::to_string(order.tp_order_price);
    }
    if (order.sl_trigger_price > 0) {
        body["slTriggerPx"] = std::to_string(order.sl_trigger_price);
        body["slOrdPx"] = std::to_string(order.sl_order_price);
    }

    json response = MakeRequest("POST", "/api/v5/trade/order", body, true);

    if (!response.empty() && response.value("code", "") == "0" &&
        response.contains("data") && !response["data"].empty()) {
        return response["data"][0].value("ordId", "");
    }

    return "";
}

std::vector<std::string> OKXRestAPI::PlaceBatchOrders(const std::vector<Order>& orders) {
    if (orders.empty() || orders.size() > 20) {
        return {};
    }

    json order_array = json::array();
    for (const auto& order : orders) {
        json order_json = {
            {"instId", order.inst_id},
            {"tdMode", order.trade_mode},
            {"side", order.side},
            {"ordType", order.order_type},
            {"sz", std::to_string(order.size)}
        };

        if (!order.position_side.empty()) {
            order_json["posSide"] = order.position_side;
        }
        if (order.price > 0) {
            order_json["px"] = std::to_string(order.price);
        }
        if (!order.client_order_id.empty()) {
            order_json["clOrdId"] = order.client_order_id;
        }

        order_array.push_back(order_json);
    }

    json response = MakeRequest("POST", "/api/v5/trade/batch-orders", order_array, true);

    std::vector<std::string> order_ids;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            if (item.value("sCode", "") == "0") {
                order_ids.push_back(item.value("ordId", ""));
            } else {
                order_ids.push_back("");  // Failed
            }
        }
    }

    return order_ids;
}

bool OKXRestAPI::CancelOrder(const std::string& inst_id,
                             const std::string& order_id,
                             const std::string& client_order_id) {
    json body = {
        {"instId", inst_id}
    };

    if (!order_id.empty()) {
        body["ordId"] = order_id;
    }
    if (!client_order_id.empty()) {
        body["clOrdId"] = client_order_id;
    }

    json response = MakeRequest("POST", "/api/v5/trade/cancel-order", body, true);

    return !response.empty() && response.value("code", "") == "0";
}

std::vector<bool> OKXRestAPI::CancelBatchOrders(const std::vector<CancelRequest>& requests) {
    if (requests.empty() || requests.size() > 20) {
        return {};
    }

    json cancel_array = json::array();
    for (const auto& req : requests) {
        json cancel_json = {
            {"instId", req.inst_id}
        };

        if (!req.order_id.empty()) {
            cancel_json["ordId"] = req.order_id;
        }
        if (!req.client_order_id.empty()) {
            cancel_json["clOrdId"] = req.client_order_id;
        }

        cancel_array.push_back(cancel_json);
    }

    json response = MakeRequest("POST", "/api/v5/trade/cancel-batch-orders", cancel_array, true);

    std::vector<bool> results;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            results.push_back(item.value("sCode", "") == "0");
        }
    }

    return results;
}

bool OKXRestAPI::AmendOrder(const std::string& inst_id,
                           const std::string& order_id,
                           const std::string& new_size,
                           const std::string& new_price) {
    json body = {
        {"instId", inst_id},
        {"ordId", order_id}
    };

    if (!new_size.empty()) {
        body["newSz"] = new_size;
    }
    if (!new_price.empty()) {
        body["newPx"] = new_price;
    }

    json response = MakeRequest("POST", "/api/v5/trade/amend-order", body, true);

    return !response.empty() && response.value("code", "") == "0";
}

Order OKXRestAPI::GetOrder(const std::string& inst_id,
                           const std::string& order_id,
                           const std::string& client_order_id) {
    json params = {
        {"instId", inst_id}
    };

    if (!order_id.empty()) {
        params["ordId"] = order_id;
    }
    if (!client_order_id.empty()) {
        params["clOrdId"] = client_order_id;
    }

    json response = MakeRequest("GET", "/api/v5/trade/order", params, true);

    Order order;

    if (!response.empty() && response.contains("data") && !response["data"].empty()) {
        order = ParseOrder(response["data"][0]);
    }

    return order;
}

std::vector<Order> OKXRestAPI::GetPendingOrders(const std::string& inst_id) {
    json params = json::object();
    if (!inst_id.empty()) {
        params["instId"] = inst_id;
    }

    json response = MakeRequest("GET", "/api/v5/trade/orders-pending", params, true);

    std::vector<Order> orders;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            orders.push_back(ParseOrder(item));
        }
    }

    return orders;
}

std::vector<Order> OKXRestAPI::GetOrderHistory(const std::string& inst_id,
                                               const std::string& state,
                                               uint64_t begin_time,
                                               uint64_t end_time,
                                               int limit) {
    json params = json::object();

    if (!inst_id.empty()) {
        params["instId"] = inst_id;
    }
    if (!state.empty()) {
        params["state"] = state;
    }
    if (begin_time > 0) {
        params["begin"] = std::to_string(begin_time);
    }
    if (end_time > 0) {
        params["end"] = std::to_string(end_time);
    }
    params["limit"] = std::to_string(limit);

    json response = MakeRequest("GET", "/api/v5/trade/orders-history", params, true);

    std::vector<Order> orders;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            orders.push_back(ParseOrder(item));
        }
    }

    return orders;
}

std::vector<Order> OKXRestAPI::GetOrderArchive(const std::string& inst_id,
                                               uint64_t begin_time,
                                               uint64_t end_time,
                                               int limit) {
    json params = json::object();

    if (!inst_id.empty()) {
        params["instId"] = inst_id;
    }
    if (begin_time > 0) {
        params["begin"] = std::to_string(begin_time);
    }
    if (end_time > 0) {
        params["end"] = std::to_string(end_time);
    }
    params["limit"] = std::to_string(limit);

    json response = MakeRequest("GET", "/api/v5/trade/orders-history-archive", params, true);

    std::vector<Order> orders;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            orders.push_back(ParseOrder(item));
        }
    }

    return orders;
}

// ==================== Trade History ====================

std::vector<OKXRestAPI::Fill> OKXRestAPI::GetFills(const std::string& inst_id,
                                                   uint64_t begin_time,
                                                   uint64_t end_time,
                                                   int limit) {
    json params = json::object();

    if (!inst_id.empty()) {
        params["instId"] = inst_id;
    }
    if (begin_time > 0) {
        params["begin"] = std::to_string(begin_time);
    }
    if (end_time > 0) {
        params["end"] = std::to_string(end_time);
    }
    params["limit"] = std::to_string(limit);

    json response = MakeRequest("GET", "/api/v5/trade/fills", params, true);

    std::vector<Fill> fills;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            Fill fill;
            fill.inst_id = item.value("instId", "");
            fill.order_id = item.value("ordId", "");
            fill.trade_id = item.value("tradeId", "");
            fill.fill_id = item.value("fillId", "");
            fill.side = item.value("side", "");
            fill.fill_price = std::stod(item.value("fillPx", "0"));
            fill.fill_size = std::stod(item.value("fillSz", "0"));
            fill.fee = std::stod(item.value("fee", "0"));
            fill.fee_currency = item.value("feeCcy", "");
            fill.fill_time = std::stoull(item.value("fillTime", "0"));
            fill.exec_type = item.value("execType", "");
            fills.push_back(fill);
        }
    }

    return fills;
}

// ==================== Bills ====================

std::vector<OKXRestAPI::Bill> OKXRestAPI::GetBills(const std::string& inst_id,
                                                   int bill_type,
                                                   uint64_t begin_time,
                                                   uint64_t end_time,
                                                   int limit) {
    json params = json::object();

    if (!inst_id.empty()) {
        params["instId"] = inst_id;
    }
    if (bill_type >= 0) {
        params["type"] = std::to_string(bill_type);
    }
    if (begin_time > 0) {
        params["begin"] = std::to_string(begin_time);
    }
    if (end_time > 0) {
        params["end"] = std::to_string(end_time);
    }
    params["limit"] = std::to_string(limit);

    json response = MakeRequest("GET", "/api/v5/account/bills", params, true);

    std::vector<Bill> bills;

    if (!response.empty() && response.contains("data")) {
        for (const auto& item : response["data"]) {
            Bill bill;
            bill.bill_id = item.value("billId", "");
            bill.inst_id = item.value("instId", "");
            bill.currency = item.value("ccy", "");
            bill.bill_type = SafeStoi(item.value("type", "0"));
            bill.bill_sub_type = item.value("subType", "");
            bill.balance_change = std::stod(item.value("balChg", "0"));
            bill.balance = std::stod(item.value("bal", "0"));
            bill.fee = std::stod(item.value("fee", "0"));
            bill.timestamp = std::stoull(item.value("ts", "0"));
            bill.notes = item.value("notes", "");
            bills.push_back(bill);
        }
    }

    return bills;
}

// ==================== Utility ====================

bool OKXRestAPI::TestConnection() {
    try {
        json response = MakeRequest("GET", "/api/v5/public/time", json::object(), false);
        return !response.empty() && response.value("code", "") == "0";
    } catch (...) {
        return false;
    }
}

OKXRestAPI::APIStatistics OKXRestAPI::GetStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    APIStatistics stats = stats_;

    if (stats.total_requests > 0) {
        stats.success_rate = static_cast<double>(stats.successful_requests) /
                            static_cast<double>(stats.total_requests) * 100.0;
    }

    if (http_client_) {
        auto http_stats = http_client_->GetStatistics();
        stats.avg_response_time_ms = http_stats.avg_response_time_ms;
    }

    return stats;
}

// ==================== Private Helper Functions ====================

json OKXRestAPI::MakeRequest(const std::string& method,
                             const std::string& endpoint,
                             const json& params,
                             bool is_private) {
    if (!initialized_) {
        std::cerr << "API not initialized" << std::endl;
        return json::object();
    }

    std::string url = config_.base_url + endpoint;
    std::string body;
    std::map<std::string, std::string> headers;

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
    }

    if (method == "GET" && !params.empty()) {
        // Add query parameters to URL
        url += "?";
        bool first = true;
        for (auto& [key, value] : params.items()) {
            if (!first) url += "&";
            url += key + "=" + value.get<std::string>();
            first = false;
        }
    } else if (method == "POST" && !params.empty()) {
        body = params.dump();
    }

    // Add authentication headers for private endpoints
    if (is_private && signer_) {
        headers = GetAuthHeaders(method, endpoint, body);
    }

    // Add simulation flag if needed
    if (config_.is_simulation) {
        headers["x-simulated-trading"] = "1";
    }

    // Make HTTP request
    HttpClient::Response response;

    if (method == "GET") {
        response = http_client_->Get(url, headers);
    } else if (method == "POST") {
        response = http_client_->Post(url, body, headers);
    } else if (method == "DELETE") {
        response = http_client_->Delete(url, headers);
    } else if (method == "PUT") {
        response = http_client_->Put(url, body, headers);
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        if (response.IsSuccess()) {
            stats_.successful_requests++;
        } else {
            stats_.failed_requests++;
        }
    }

    // Parse JSON response
    if (response.IsSuccess() && !response.body.empty()) {
        try {
            return json::parse(response.body);
        } catch (const json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            std::cerr << "Response: " << response.body << std::endl;
            return json::object();
        }
    }

    if (!response.IsSuccess()) {
        std::cerr << "HTTP request failed: " << response.status_code << std::endl;
        std::cerr << "Response: " << response.body << std::endl;
    }

    return json::object();
}

std::map<std::string, std::string> OKXRestAPI::GetAuthHeaders(
    const std::string& method,
    const std::string& request_path,
    const std::string& body) {

    std::string timestamp = OKXSigner::GetTimestamp();
    std::string signature = signer_->Sign(timestamp, method, request_path, body);

    return {
        {"OK-ACCESS-KEY", signer_->GetAPIKey()},
        {"OK-ACCESS-SIGN", signature},
        {"OK-ACCESS-TIMESTAMP", timestamp},
        {"OK-ACCESS-PASSPHRASE", signer_->GetPassphrase()}
    };
}

// ==================== Parse Response Helpers ====================

Tick OKXRestAPI::ParseTicker(const json& data) {
    Tick tick;

    tick.inst_id = data.value("instId", "");
    tick.last_price = std::stod(data.value("last", "0"));
    tick.bid_price = std::stod(data.value("bidPx", "0"));
    tick.bid_size = std::stod(data.value("bidSz", "0"));
    tick.ask_price = std::stod(data.value("askPx", "0"));
    tick.ask_size = std::stod(data.value("askSz", "0"));
    tick.high_24h = std::stod(data.value("high24h", "0"));
    tick.low_24h = std::stod(data.value("low24h", "0"));
    tick.volume_24h = std::stod(data.value("vol24h", "0"));
    tick.volume_currency_24h = std::stod(data.value("volCcy24h", "0"));
    tick.timestamp = std::stoull(data.value("ts", "0"));

    return tick;
}

Depth OKXRestAPI::ParseOrderBook(const json& data) {
    Depth depth;

    depth.timestamp = std::stoull(data.value("ts", "0"));

    // Parse bids
    if (data.contains("bids") && data["bids"].is_array()) {
        for (const auto& bid : data["bids"]) {
            if (bid.is_array() && bid.size() >= 2) {
                DepthLevel level;
                level.price = std::stod(bid[0].get<std::string>());
                level.size = std::stod(bid[1].get<std::string>());
                depth.bids.push_back(level);
            }
        }
    }

    // Parse asks
    if (data.contains("asks") && data["asks"].is_array()) {
        for (const auto& ask : data["asks"]) {
            if (ask.is_array() && ask.size() >= 2) {
                DepthLevel level;
                level.price = std::stod(ask[0].get<std::string>());
                level.size = std::stod(ask[1].get<std::string>());
                depth.asks.push_back(level);
            }
        }
    }

    return depth;
}

Order OKXRestAPI::ParseOrder(const json& data) {
    Order order;

    order.order_id = data.value("ordId", "");
    order.client_order_id = data.value("clOrdId", "");
    order.inst_id = data.value("instId", "");
    order.side = data.value("side", "");
    order.position_side = data.value("posSide", "");
    order.order_type = data.value("ordType", "");
    order.trade_mode = data.value("tdMode", "");
    order.price = std::stod(data.value("px", "0"));
    order.size = std::stod(data.value("sz", "0"));
    order.filled_size = std::stod(data.value("accFillSz", "0"));
        order.avg_fill_price = std::stod(data.value("avgPx", "0"));
    order.state = data.value("state", "");
    order.fee = std::stod(data.value("fee", "0"));
    order.pnl = std::stod(data.value("pnl", "0"));
    order.create_time = std::stoull(data.value("cTime", "0"));
    order.update_time = std::stoull(data.value("uTime", "0"));

    // Optional fields
    if (data.contains("lever")) {
        order.leverage = SafeStoi(data["lever"].get<std::string>());
    }
    if (data.contains("tpTriggerPx")) {
        order.tp_trigger_price = std::stod(data["tpTriggerPx"].get<std::string>());
    }
    if (data.contains("tpOrdPx")) {
        order.tp_order_price = std::stod(data["tpOrdPx"].get<std::string>());
    }
    if (data.contains("slTriggerPx")) {
        order.sl_trigger_price = std::stod(data["slTriggerPx"].get<std::string>());
    }
    if (data.contains("slOrdPx")) {
        order.sl_order_price = std::stod(data["slOrdPx"].get<std::string>());
    }

    return order;
}

Position OKXRestAPI::ParsePosition(const json& data) {
    Position pos;

    pos.inst_id = data.value("instId", "");
    pos.position_side = data.value("posSide", "");
    pos.position = std::stod(data.value("pos", "0"));
    pos.available_position = std::stod(data.value("availPos", "0"));
    pos.avg_price = std::stod(data.value("avgPx", "0"));
    pos.mark_price = std::stod(data.value("markPx", "0"));
    pos.liquidation_price = std::stod(data.value("liqPx", "0"));
    pos.unrealized_pnl = std::stod(data.value("upl", "0"));
    pos.unrealized_pnl_ratio = std::stod(data.value("uplRatio", "0"));
    pos.leverage = SafeStoi(data.value("lever", "0"));
    pos.margin = std::stod(data.value("margin", "0"));
    pos.margin_ratio = std::stod(data.value("mgnRatio", "0"));
    pos.initial_margin = std::stod(data.value("imr", "0"));
    pos.maintenance_margin = std::stod(data.value("mmr", "0"));
    pos.trade_mode = data.value("mgnMode", "");
    pos.create_time = std::stoull(data.value("cTime", "0"));
    pos.update_time = std::stoull(data.value("uTime", "0"));

    return pos;
}

Account OKXRestAPI::ParseAccount(const json& data) {
    Account account;

    // 第889-895行 - 全部改为 SafeStod/SafeStoull
    account.total_equity = SafeStod(data.value("totalEq", "0"));              // ✅
    account.isolated_equity = SafeStod(data.value("isoEq", "0"));            // ✅
    account.adj_equity = SafeStod(data.value("adjEq", "0"));                 // ✅
    account.margin_ratio = SafeStod(data.value("mgnRatio", "0"));            // ✅
    account.maintenance_margin_ratio = SafeStod(data.value("mmr", "0"));     // ✅
    account.initial_margin_ratio = SafeStod(data.value("imr", "0"));         // ✅
    account.update_time = SafeStoull(data.value("uTime", "0"));              // ✅

    // Parse details (currency-specific balances)
    if (data.contains("details") && data["details"].is_array()) {
        for (const auto& detail : data["details"]) {
            Account::Detail d;
            d.currency = detail.value("ccy", "");
                d.equity = SafeStod(detail.value("eq", "0"));                            // ✅
                d.cash_balance = SafeStod(detail.value("cashBal", "0"));                 // ✅
                d.available_balance = SafeStod(detail.value("availBal", "0"));           // ✅
                d.frozen_balance = SafeStod(detail.value("frozenBal", "0"));             // ✅
                d.order_frozen = SafeStod(detail.value("ordFrozen", "0"));               // ✅
                d.available_equity = SafeStod(detail.value("availEq", "0"));             // ✅
                d.unrealized_pnl = SafeStod(detail.value("upl", "0"));                   // ✅
                account.details.push_back(d);
        }
    }

    return account;
}
