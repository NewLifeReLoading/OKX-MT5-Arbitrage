#include "config.h"
#include <fstream>
#include <iostream>

bool Config::Load(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filepath << std::endl;
            return false;
        }
        
        file >> config_;
        environment_ = config_["environment"].get<std::string>();
        
        std::cout << "Config loaded successfully, environment: " << environment_ << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return false;
    }
}

// OKX配置
std::string Config::GetOKXAPIKey() const {
    return config_["okx"][environment_]["api_key"].get<std::string>();
}

std::string Config::GetOKXSecretKey() const {
    return config_["okx"][environment_]["secret_key"].get<std::string>();
}

std::string Config::GetOKXPassphrase() const {
    return config_["okx"][environment_]["passphrase"].get<std::string>();
}

std::string Config::GetOKXRESTURL() const {
    return config_["okx"][environment_]["rest_url"].get<std::string>();
}

std::string Config::GetOKXWSPublic() const {
    return config_["okx"][environment_]["ws_public"].get<std::string>();
}

std::string Config::GetOKXWSPrivate() const {
    return config_["okx"][environment_]["ws_private"].get<std::string>();
}

std::string Config::GetOKXSymbol(const std::string& base) const {
    return config_["okx"]["symbols"][base].get<std::string>();
}

// MT5配置
std::string Config::GetMT5Server() const {
    return config_["mt5"][environment_]["server"].get<std::string>();
}

int Config::GetMT5Login() const {
    return config_["mt5"][environment_]["login"].get<int>();
}

std::string Config::GetMT5Password() const {
    return config_["mt5"][environment_]["password"].get<std::string>();
}

std::string Config::GetMT5Symbol(const std::string& base) const {
    return config_["mt5"]["symbols"][base].get<std::string>();
}

// 策略配置
double Config::GetFirstOrder() const {
    return config_["strategy"]["first_order"].get<double>();
}

double Config::GetNextOrder() const {
    return config_["strategy"]["next_order"].get<double>();
}

int Config::GetMaxOrders() const {
    return config_["strategy"]["max_orders"].get<int>();
}

double Config::GetTakeProfit() const {
    return config_["strategy"]["take_profit"].get<double>();
}

double Config::GetOKXFeeRate() const {
    return config_["strategy"]["okx_fee_rate"].get<double>();
}

double Config::GetMT5FeeRate() const {
    return config_["strategy"]["mt5_fee_rate"].get<double>();
}

double Config::GetOKXOrderSize() const {
    return config_["strategy"]["okx_order_size"].get<double>();
}

double Config::GetMT5OrderSize() const {
    return config_["strategy"]["mt5_order_size"].get<double>();
}
