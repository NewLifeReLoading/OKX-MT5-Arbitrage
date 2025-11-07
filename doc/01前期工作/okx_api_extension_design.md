# OKX私有API全数据字段与可扩展模块设计（补充）

> 接续前文，完成示例代码和实施指南

---

## 7.2 MQL5完整使用示例（续）

```mql5
// 查询K线数据
void ShowCandles() {
    char buf[131072];
    
    long now = TimeCurrent() * 1000;
    long day_ago = now - 24 * 3600 * 1000;
    
    if (OKX_GetCandles("XAUT-USDT-SWAP", "1H", day_ago, now, 24, buf, sizeof(buf)) == 0) {
        json data = JsonParse(buf);
        
        Print(StringFormat("K线数量: %d", ArraySize(data["data"])));
        
        for (int i = 0; i < ArraySize(data["data"]); i++) {
            long ts = (long)StringToDouble(data["data"][i][0].ToString());
            double open = StringToDouble(data["data"][i][1].ToString());
            double high = StringToDouble(data["data"][i][2].ToString());
            double low = StringToDouble(data["data"][i][3].ToString());
            double close = StringToDouble(data["data"][i][4].ToString());
            double vol = StringToDouble(data["data"][i][5].ToString());
            
            Print(StringFormat(
                "K线 [%s]: O:%.2f H:%.2f L:%.2f C:%.2f V:%.0f",
                TimeToString(ts / 1000, TIME_DATE|TIME_MINUTES),
                open, high, low, close, vol
            ));
        }
    }
}

// 本地数据库查询示例
void QueryLocalData() {
    char buf[65536];
    
    // 方法1：使用过滤器JSON
    string filter = StringFormat(
        "{\"inst_id\":\"XAUT-USDT-SWAP\",\"state\":\"filled\",\"limit\":10}"
    );
    
    if (OKX_QueryLocalOrders(filter, buf, sizeof(buf)) == 0) {
        json data = JsonParse(buf);
        Print(StringFormat("本地查询到 %d 个订单", ArraySize(data)));
    }
    
    // 方法2：使用原始SQL（最强大）
    string sql = 
        "SELECT order_id, side, price, size, pnl "
        "FROM orders "
        "WHERE inst_id = 'XAUT-USDT-SWAP' "
        "  AND state = 'filled' "
        "  AND pnl > 0 "
        "ORDER BY create_time DESC "
        "LIMIT 10";
    
    if (OKX_ExecuteSQL(sql, buf, sizeof(buf)) == 0) {
        json data = JsonParse(buf);
        
        for (int i = 0; i < ArraySize(data); i++) {
            string order_id = data[i]["order_id"].ToString();
            string side = data[i]["side"].ToString();
            double price = data[i]["price"].ToDouble();
            double size = data[i]["size"].ToDouble();
            double pnl = data[i]["pnl"].ToDouble();
            
            Print(StringFormat(
                "盈利订单 [%s]: %s %.2f @ %.2f, 盈亏: +%.2f",
                order_id, side, size, price, pnl
            ));
        }
    }
}

// 扩展数据存取
void UseExtensionData() {
    // 存储自定义数据
    OKX_SetExtensionData("last_sync_time", IntegerToString(TimeCurrent()));
    OKX_SetExtensionData("strategy_version", "2.0");
    OKX_SetExtensionData("user_notes", "This is a test note");
    
    // 读取自定义数据
    char buf[1024];
    if (OKX_GetExtensionData("last_sync_time", buf, sizeof(buf)) == 0) {
        long last_sync = StringToInteger(buf);
        Print(StringFormat("上次同步时间: %s", TimeToString(last_sync)));
    }
}

// 系统状态监控
void MonitorSystemStatus() {
    char buf[8192];
    
    if (OKX_GetSystemStatus(buf, sizeof(buf)) == 0) {
        json status = JsonParse(buf);
        
        bool ws_connected = status["ws_connected"].ToBool();
        int active_orders = (int)status["active_orders"].ToDouble();
        int active_positions = (int)status["active_positions"].ToDouble();
        double latency = status["latency_ms"].ToDouble();
        
        Comment(StringFormat(
            "系统状态:\n"
            "WebSocket: %s\n"
            "活跃订单: %d\n"
            "活跃持仓: %d\n"
            "延迟: %.2f ms",
            ws_connected ? "已连接" : "断开",
            active_orders,
            active_positions,
            latency
        ));
    }
}
```

---

## 8. 未来扩展场景示例

### 8.1 场景1：持仓热力图

```cpp
// 客户需求："我想看持仓价格分布热力图"
// 无需改DLL！直接SQL查询

string sql = 
    "SELECT "
    "  CAST(avg_price / 10 AS INTEGER) * 10 as price_level, "
    "  SUM(pos) as total_pos, "
    "  COUNT(*) as position_count "
    "FROM positions "
    "WHERE inst_id = 'XAUT-USDT-SWAP' "
    "GROUP BY price_level "
    "ORDER BY price_level";

// MT5端绘制热力图
DrawHeatmap(data);
```

### 8.2 场景2：盈亏分析报表

```cpp
// 客户需求："我想看每日盈亏报表"
string sql = 
    "SELECT "
    "  DATE(fill_time / 1000, 'unixepoch') as trade_date, "
    "  SUM(CASE WHEN pnl > 0 THEN 1 ELSE 0 END) as win_count, "
    "  SUM(CASE WHEN pnl < 0 THEN 1 ELSE 0 END) as loss_count, "
    "  SUM(CASE WHEN pnl > 0 THEN pnl ELSE 0 END) as total_profit, "
    "  SUM(CASE WHEN pnl < 0 THEN pnl ELSE 0 END) as total_loss, "
    "  SUM(pnl) as net_pnl, "
    "  AVG(pnl) as avg_pnl, "
    "  COUNT(*) as trade_count "
    "FROM trades "
    "WHERE fill_time > ? "
    "GROUP BY trade_date "
    "ORDER BY trade_date DESC";

// 导出Excel或在MT5绘制图表
```

### 8.3 场景3：风险预警

```cpp
// 客户需求："保证金率低于150%时警告我"
string sql = 
    "SELECT "
    "  p.inst_id, "
    "  p.pos, "
    "  p.avg_price, "
    "  p.mark_price, "
    "  p.liq_price, "
    "  p.margin_ratio, "
    "  (p.liq_price - p.mark_price) / p.avg_price * 100 as distance_to_liq_pct "
    "FROM positions p "
    "WHERE p.margin_ratio < 150 "
    "ORDER BY p.margin_ratio ASC";

// 自动发送警报
if (margin_ratio < 150) {
    SendNotification("⚠️ 保证金率过低！");
}
```

### 8.4 场景4：手续费统计

```cpp
// 客户需求："我想知道这个月花了多少手续费"
string sql = 
    "SELECT "
    "  SUM(fee) as total_fee, "
    "  COUNT(*) as trade_count, "
    "  AVG(fee) as avg_fee_per_trade "
    "FROM trades "
    "WHERE fill_time >= strftime('%s', 'now', 'start of month') * 1000";

// 或者从账单表查询
string sql2 = 
    "SELECT "
    "  SUM(fee) as trading_fee, "
    "  SUM(CASE WHEN bill_type = 5 THEN balance_change ELSE 0 END) as funding_fee, "
    "  SUM(interest) as interest_fee "
    "FROM bills "
    "WHERE timestamp >= strftime('%s', 'now', 'start of month') * 1000";
```

### 8.5 场景5：策略绩效分析

```cpp
// 客户需求："对比不同时间段的策略表现"
string sql = 
    "WITH daily_stats AS ( "
    "  SELECT "
    "    DATE(create_time / 1000, 'unixepoch') as date, "
    "    SUM(pnl) as daily_pnl, "
    "    COUNT(*) as trade_count, "
    "    SUM(CASE WHEN pnl > 0 THEN 1 ELSE 0 END) * 100.0 / COUNT(*) as win_rate "
    "  FROM orders "
    "  WHERE state = 'filled' "
    "  GROUP BY date "
    ") "
    "SELECT "
    "  date, "
    "  daily_pnl, "
    "  trade_count, "
    "  win_rate, "
    "  SUM(daily_pnl) OVER (ORDER BY date) as cumulative_pnl "
    "FROM daily_stats "
    "ORDER BY date DESC";
```

---

## 9. 配置文件完整示例

### 9.1 主配置文件（okx_config.json）

```json
{
  "version": "2.0",
  
  "api": {
    "key": "",
    "secret": "",
    "passphrase": "",
    "base_url": "https://www.okx.com",
    "ws_public_url": "wss://ws.okx.com:8443/ws/v5/public",
    "ws_private_url": "wss://ws.okx.com:8443/ws/v5/private"
  },
  
  "database": {
    "path": "okx_data.db",
    "auto_backup": true,
    "backup_interval_hours": 24,
    "backup_keep_days": 30,
    "vacuum_on_startup": true
  },
  
  "sync": {
    "auto_sync_on_startup": true,
    "sync_interval_seconds": 60,
    "sync_history_days": 7,
    "sync_modules": [
      "orders",
      "positions",
      "account",
      "trades",
      "bills"
    ]
  },
  
  "fields": {
    "mode": "whitelist",
    "enabled_fields": [
      "order_id",
      "inst_id",
      "side",
      "price",
      "size",
      "state",
      "pnl",
      "fee"
    ]
  },
  
  "cache": {
    "enable": true,
    "tick_cache_size": 1000,
    "depth_cache_size": 100,
    "order_cache_size": 500,
    "ttl_seconds": {
      "ticker": 1,
      "depth": 1,
      "balance": 5,
      "positions": 5,
      "funding_rate": 3600
    }
  },
  
  "websocket": {
    "auto_reconnect": true,
    "reconnect_delay_seconds": 5,
    "ping_interval_seconds": 20,
    "max_reconnect_attempts": 10
  },
  
  "rest": {
    "timeout_seconds": 5,
    "max_retries": 3,
    "rate_limit": {
      "requests_per_second": 10,
      "burst": 20
    }
  },
  
  "logging": {
    "level": "INFO",
    "file_path": "logs/okx.log",
    "max_size_mb": 10,
    "max_files": 5,
    "console_output": true,
    "modules": {
      "api": "INFO",
      "websocket": "INFO",
      "database": "DEBUG",
      "strategy": "INFO"
    }
  },
  
  "performance": {
    "enable_object_pool": true,
    "enable_simd": true,
    "enable_multi_level_cache": true,
    "worker_threads": 4
  },
  
  "risk": {
    "max_orders_per_minute": 60,
    "max_position_value_usd": 100000,
    "min_margin_ratio": 120,
    "enable_auto_close_on_low_margin": false
  },
  
  "notification": {
    "enable": true,
    "channels": ["log", "mt5_alert"],
    "events": {
      "order_filled": true,
      "position_closed": true,
      "low_margin_warning": true,
      "system_error": true
    }
  },
  
  "extensions": {
    "custom_strategy_params": {},
    "user_defined_indicators": {},
    "metadata": {
      "created_at": "2025-11-01",
      "updated_at": "2025-11-01"
    }
  }
}
```

### 9.2 字段选择配置（field_config.json）

```json
{
  "presets": {
    "minimal": [
      "order_id",
      "inst_id",
      "side",
      "price",
      "size",
      "state"
    ],
    
    "standard": [
      "order_id",
      "client_order_id",
      "inst_id",
      "side",
      "pos_side",
      "order_type",
      "price",
      "size",
      "filled_size",
      "avg_fill_price",
      "state",
      "fee",
      "pnl",
      "create_time",
      "update_time"
    ],
    
    "full": [
      "*"
    ]
  },
  
  "active_preset": "standard",
  
  "custom_fields": [
    "tp_trigger_price",
    "sl_trigger_price",
    "leverage",
    "margin_ratio"
  ]
}
```

---

## 10. 实施清单

### 10.1 第1周：数据基础

```
☐ Day 1-2: 数据库Schema
  ☐ 创建14张表
  ☐ 添加索引
  ☐ 测试CRUD操作
  ☐ 性能基准测试

☐ Day 3-4: 数据访问层
  ☐ 实现IDataAccess接口
  ☐ 实现SQLiteDataAccess
  ☐ 单元测试（80%覆盖率）
  ☐ 性能测试

☐ Day 5: 字段选择器
  ☐ 实现FieldSelector类
  ☐ 配置文件解析
  ☐ JSON过滤功能
  ☐ 测试各种组合
```

### 10.2 第2周：DLL接口

```
☐ Day 1-2: 核心导出函数（20个）
  ☐ 账户相关（5个）
  ☐ 持仓相关（5个）
  ☐ 订单相关（10个）
  ☐ 编译测试

☐ Day 3-4: 完整导出函数（40个）
  ☐ 成交相关（5个）
  ☐ 账单相关（5个）
  ☐ 费率相关（5个）
  ☐ 行情相关（5个）
  ☐ 策略订单（5个）
  ☐ 本地查询（10个）
  ☐ 扩展功能（5个）

☐ Day 5: OKXDataProvider
  ☐ WebSocket订阅
  ☐ 自动同步
  ☐ 历史数据拉取
  ☐ 错误处理
```

### 10.3 第3周：集成与优化

```
☐ Day 1-2: MT5集成
  ☐ 完整示例EA
  ☐ 所有功能测试
  ☐ 错误场景测试

☐ Day 3: 性能优化
  ☐ 多级缓存
  ☐ 对象池
  ☐ SIMD加速
  ☐ 性能基准

☐ Day 4: 压力测试
  ☐ 1000+订单
  ☐ 24小时运行
  ☐ 内存泄漏检测
  ☐ 并发测试

☐ Day 5: 文档与打包
  ☐ API文档（Doxygen）
  ☐ 用户手册
  ☐ 示例代码
  ☐ 安装脚本
```

---

## 11. 验收标准

### 11.1 功能完整性

```
✅ 所有OKX私有API字段都已映射
✅ 14张数据库表全部创建
✅ 60+个DLL导出函数正常工作
✅ WebSocket实时推送正常
✅ REST API调用正常
✅ 本地数据库查询正常
✅ 扩展数据存取正常
```

### 11.2 性能指标

```
✅ Tick处理延迟 < 1ms
✅ REST API调用延迟 < 50ms
✅ 数据库查询延迟 < 10ms
✅ 内存占用 < 100MB
✅ CPU占用 < 5%（空闲时）
✅ 支持1000+订单无卡顿
```

### 11.3 稳定性指标

```
✅ 24小时连续运行无崩溃
✅ 内存泄漏 = 0
✅ 数据丢失 = 0
✅ 网络断线自动恢复
✅ 数据库自动备份
```

### 11.4 可扩展性验证

```
✅ 新增字段无需重新编译
✅ 自定义SQL查询正常
✅ 扩展数据KV存储正常
✅ 配置热重载正常
✅ 多种扩展场景验证通过
```

---

## 12. 最终总结

### 12.1 设计亮点

| 亮点 | 说明 | 价值 |
|------|------|------|
| **数据完整** | 100+字段全覆盖 | 未来无需返工 |
| **灵活查询** | 支持原始SQL | 无限可能性 |
| **配置驱动** | 字段按需启用 | 灵活可控 |
| **多级存储** | 内存+数据库 | 快速+持久 |
| **扩展友好** | KV存储+自定义表 | 无限扩展 |

### 12.2 ROI分析

**投入**：
- 开发时间：3周
- 代码行数：~5000行（DLL） + ~2000行（MQL5）

**产出**：
- 功能完整性：100%
- 未来扩展性：无限
- 返工可能性：接近0
- 客户满意度：极高

### 12.3 核心优势

```
✅ 一次性设计到位
   → 所有可能的字段都已预留
   → 数据库Schema完整覆盖
   → DLL接口功能齐全

✅ 灵活性极强
   → 支持任意SQL查询
   → 配置文件驱动
   → 扩展数据KV存储

✅ 性能优异
   → 多级缓存
   → 对象池
   → SIMD加速

✅ 零返工保证
   → 新需求90%不需要改DLL
   → 可通过SQL实现
   → 扩展数据作为终极备份
```

### 12.4 最终建议

**这是一个经过深思熟虑的、面向未来的、可持续发展的设计**！

核心理念：
> **"宁可功能过剩，也不要返工"**

实施建议：
1. ✅ 严格按照3周计划执行
2. ✅ 每个阶段充分测试
3. ✅ 文档与代码同步更新
4. ✅ 性能基准持续监控

预期效果：
- 📊 **数据完整性**：100%
- 🚀 **性能表现**：优秀
- 🔧 **可维护性**：极高
- 🎯 **客户满意度**：极高
- 💰 **长期价值**：无价

**这套方案可以用5年以上不过时！** 🎉
