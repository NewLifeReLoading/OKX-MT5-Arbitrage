#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief 简单的配置管理器
 */
class Config {
public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }
    
    // 加载配置文件
    bool Load(const std::string& filepath);
    
    // 获取当前环境
    bool IsSimulation() const { return environment_ == "simulation"; }
    std::string GetEnvironment() const { return environment_; }
    
    // 切换环境
    void SetEnvironment(const std::string& env) { environment_ = env; }
    
    // OKX配置
    std::string GetOKXAPIKey() const;
    std::string GetOKXSecretKey() const;
    std::string GetOKXPassphrase() const;
    std::string GetOKXRESTURL() const;
    std::string GetOKXWSPublic() const;
    std::string GetOKXWSPrivate() const;
    std::string GetOKXSymbol(const std::string& base) const;
    
    // MT5配置
    std::string GetMT5Server() const;
    int GetMT5Login() const;
    std::string GetMT5Password() const;
    std::string GetMT5Symbol(const std::string& base) const;
    
    // 策略配置
    double GetFirstOrder() const;
    double GetNextOrder() const;
    int GetMaxOrders() const;
    double GetTakeProfit() const;
    double GetOKXFeeRate() const;
    double GetMT5FeeRate() const;
    double GetOKXOrderSize() const;
    double GetMT5OrderSize() const;
    
private:
    Config() = default;
    json config_;
    std::string environment_;
};
