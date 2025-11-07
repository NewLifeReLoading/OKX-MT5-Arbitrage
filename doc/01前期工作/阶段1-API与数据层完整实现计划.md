# 🎯 阶段1：API与数据层完整实现计划

> **核心目标**: 构建高质量、可验证、易扩展的数据底层  
> **验证方式**: 完整的API验证工具  
> **开发模式**: 边制作、边验证、边优化

---

## 📋 阶段1总览

### 核心目标

```
┌─────────────────────────────────────────────┐
│           API与数据层（第一阶段）            │
│                                              │
│  目标1: OKX API全字段完整获取               │
│  目标2: 数据库全字段完整存储                │
│  目标3: DLL接口全功能导出                   │
│  目标4: 完整的验证工具                      │
│  目标5: 高度解耦、易扩展                    │
│                                              │
│  验证标准: 一个工具能展示所有数据！         │
└─────────────────────────────────────────────┘
```

### 不包含内容（后续阶段）

❌ 策略引擎（套利逻辑）  
❌ 盈亏计算器  
❌ 图表面板  
❌ 订单面板  
❌ 套利交易功能  

### 包含内容（本阶段）

✅ **OKX REST API** - 完整封装，全字段支持  
✅ **OKX WebSocket** - 实时推送，全频道订阅  
✅ **SQLite数据库** - 14张表，100+字段  
✅ **DLL导出接口** - 60+个函数  
✅ **API验证工具** - 完整展示所有数据  
✅ **配置系统** - 灵活可配置  
✅ **日志系统** - 完整日志记录  

---

## 🏗️ 模块架构图

```
┌─────────────────────────────────────────────────────────┐
│                   MT5 验证工具（EA）                     │
│  ┌───────────────────────────────────────────────────┐ │
│  │  数据展示面板                                      │ │
│  │  - 账户信息展示（所有字段）                       │ │
│  │  - 持仓信息展示（所有字段）                       │ │
│  │  - 订单信息展示（所有字段）                       │ │
│  │  - 成交记录展示（所有字段）                       │ │
│  │  - 账单记录展示（所有字段）                       │ │
│  │  - 行情数据展示（Tick/深度/K线）                  │ │
│  │  - 资金费率展示                                    │ │
│  │  - 系统状态展示                                    │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                          ↕ (DLL调用)
┌─────────────────────────────────────────────────────────┐
│                    DLL导出层                             │
│  ┌───────────────────────────────────────────────────┐ │
│  │  60+ 导出函数                                      │ │
│  │  - 初始化/关闭接口（5个）                         │ │
│  │  - 账户查询接口（8个）                            │ │
│  │  - 持仓查询接口（8个）                            │ │
│  │  - 订单操作接口（10个）                           │ │
│  │  - 成交查询接口（5个）                            │ │
│  │  - 账单查询接口（5个）                            │ │
│  │  - 行情查询接口（8个）                            │ │
│  │  - 本地数据库查询（8个）                          │ │
│  │  - 系统管理接口（5个）                            │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                          ↕
┌─────────────────────────────────────────────────────────┐
│                  业务逻辑层（C++）                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ OKXAPIClient │  │WebSocketClient│  │ DataProvider │ │
│  │ - REST API   │  │ - 公共频道   │  │ - 数据聚合   │ │
│  │ - 签名算法   │  │ - 私有频道   │  │ - 缓存管理   │ │
│  │ - 错误处理   │  │ - 自动重连   │  │ - 同步逻辑   │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
│                          ↕                               │
│  ┌──────────────────────────────────────────────────┐  │
│  │           DatabaseManager（SQLite）               │  │
│  │  - 14张数据表（100+字段）                        │  │
│  │  - CRUD操作                                       │  │
│  │  - 原始SQL查询支持                                │  │
│  │  - 自动备份                                       │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                          ↕
┌─────────────────────────────────────────────────────────┐
│                  配置与工具层                            │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────┐ │
│  │ConfigManager│  │LogManager  │  │  FieldSelector   │ │
│  │- JSON配置  │  │- 分级日志  │  │- 字段过滤       │ │
│  │- 热重载    │  │- 文件滚动  │  │- 按需启用字段   │ │
│  └────────────┘  └────────────┘  └──────────────────┘ │
└─────────────────────────────────────────────────────────┘
                          ↕
┌─────────────────────────────────────────────────────────┐
│                    外部系统                              │
│              OKX交易所 REST API + WebSocket             │
└─────────────────────────────────────────────────────────┘
```

---

## 📦 详细模块清单

### 模块M1：数据结构定义 ✅（已完成）

**状态**: 已完成基础结构  
**需要补充**: OKX API全字段结构

#### 新增内容

```cpp
// okx_types.h - OKX API完整字段定义

// 账户配置
struct AccountConfig {
    string uid;              // 用户ID
    string acct_lv;          // 账户层级 1:简单 2:单币种 3:跨币种 4:组合保证金
    string pos_mode;         // 持仓模式 long_short_mode净持仓 net_mode双向持仓
    bool auto_loan;          // 是否自动借币
    string greeks_type;      // 希腊字母展示方式
    string level;            // 账户等级
    string level_tmp;        // 临时体验的账户等级
    string ct_iso_mode;      // 合约逐仓保证金划转模式
    string mg_iso_mode;      // 币币杠杆逐仓保证金划转模式
    string spot_offset_type; // 现货对冲模式
    string role_type;        // 角色类型 0:普通用户 1:做市商等
    string trade_role;       // 交易角色
    // ... 更多字段
};

// 账户余额详细信息
struct AccountBalance {
    string ccy;              // 币种
    string eq;               // 币种总权益
    string cash_bal;         // 币种余额
    string upl;              // 未实现盈亏
    string avail_eq;         // 可用保证金
    string dis_eq;           // 美元层面币种折算权益
    string avail_bal;        // 可用余额
    string frozen_bal;       // 冻结余额
    string ord_frozen;       // 挂单冻结数量
    string liab;             // 负债额
    string upl_liab;         // 未实现盈亏负债额
    string cross_liab;       // 全仓负债额
    string iso_liab;         // 逐仓负债额
    string margin_ratio;     // 保证金率
    string interest;         // 利息
    string twap;             // 当前负债币种触发系统自动换币的风险
    string max_loan;         // 币种最大可借
    string eq_usd;           // 币种权益美元价值
    string notional_lever;   // 杠杆倍数
    string stgy_eq;          // 策略权益
    string iso_upl;          // 逐仓未实现盈亏
    string spot_in_use_amt;  // 现货对冲占用数量
    string spot_iso_upl;     // 现货对冲占用部分的未实现盈亏
    // ... 更多字段
};

// 持仓详细信息（全字段）
struct PositionDetail {
    string inst_type;        // 产品类型 MARGIN SWAP FUTURES OPTION
    string inst_id;          // 产品ID
    string pos_id;           // 持仓ID
    string pos_side;         // 持仓方向 long short net
    string pos;              // 持仓数量
    string avail_pos;        // 可平仓数量
    string avg_px;           // 开仓平均价
    string mark_px;          // 标记价格
    string upl;              // 未实现收益
    string upl_ratio;        // 未实现收益率
    string upl_last_px;      // 以最新成交价计算的未实现收益
    string upl_ratio_last_px;// 以最新成交价计算的未实现收益率
    string inst_type_px;     // 产品类型价格
    string notional_usd;     // 以美元价值为单位的持仓数量
    string adl;              // 信号区 1,2,3,4,5
    string imr;              // 初始保证金
    string margin;           // 保证金余额
    string margin_ratio;     // 保证金率
    string mmr;              // 维持保证金
    string liab;             // 负债额
    string liab_ccy;         // 负债币种
    string interest;         // 利息
    string trade_id;         // 最新成交ID
    string opt_val;          // 期权市值
    string pending_close_ord_liab_val; // 平仓挂单的负债额
    string notional_ccy;     // 交易货币持仓数量
    string liq_px;           // 预估强平价
    string lever;            // 杠杆倍数
    string delta_bs;         // delta 用于期权
    string delta_pa;         // delta PA 用于期权
    string gamma_bs;         // gamma 用于期权
    string gamma_pa;         // gamma PA 用于期权
    string theta_bs;         // theta 用于期权
    string theta_pa;         // theta PA 用于期权
    string vega_bs;          // vega 用于期权
    string vega_pa;          // vega PA 用于期权
    string spot_in_use_amt;  // 现货对冲占用数量
    string spot_in_use_ccy;  // 现货对冲占用币种
    string clSpotInUseAmt;   // 用户自定义现货对冲占用数量
    string max_spot_in_use_amt; // 现货对冲占用数量上限
    string biz_ref_id;       // 业务线ID
    string biz_ref_type;     // 业务线类型
    string base_bal;         // 交易货币余额
    string quote_bal;        // 计价货币余额
    string base_borrowed;    // 交易货币已借
    string base_interest;    // 交易货币利息
    string quote_borrowed;   // 计价货币已借
    string quote_interest;   // 计价货币利息
    string pos_ccy;          // 盈亏结算币种
    string fee;              // 累计手续费
    string funding_fee;      // 累计资金费用
    string liq_penalty;      // 累计强平罚金
    string realized_pnl;     // 已实现收益
    string pnl;              // 收益
    string close_order_algo; // 平仓策略委托订单
    string c_time;           // 持仓创建时间
    string u_time;           // 持仓更新时间
    string px_usd;           // 美元价格
    
    // 自定义扩展字段
    Timestamp create_time_ms;
    Timestamp update_time_ms;
    double upl_numeric;
    double pnl_numeric;
    double margin_ratio_numeric;
};

// 订单详细信息（全字段）
struct OrderDetail {
    string inst_type;        // 产品类型
    string inst_id;          // 产品ID
    string ccy;              // 保证金币种
    string ord_id;           // 订单ID
    string cl_ord_id;        // 客户自定义订单ID
    string tag;              // 订单标签
    string px;               // 委托价格
    string sz;               // 委托数量
    string pnl;              // 收益
    string ord_type;         // 订单类型 market/limit/post_only等
    string side;             // 订单方向 buy/sell
    string pos_side;         // 持仓方向
    string td_mode;          // 交易模式 isolated/cross/cash
    string acc_fill_sz;      // 累计成交数量
    string fill_px;          // 最新成交价格
    string trade_id;         // 最新成交ID
    string fill_sz;          // 最新成交数量
    string fill_time;        // 最新成交时间
    string state;            // 订单状态
    string avg_px;           // 成交均价
    string lever;            // 杠杆倍数
    string tp_trigger_px;    // 止盈触发价
    string tp_trigger_px_type; // 止盈触发价类型
    string tp_ord_px;        // 止盈委托价
    string sl_trigger_px;    // 止损触发价
    string sl_trigger_px_type; // 止损触发价类型
    string sl_ord_px;        // 止损委托价
    string fee_ccy;          // 交易手续费币种
    string fee;              // 订单交易手续费
    string rebate_ccy;       // 返佣币种
    string rebate;           // 返佣金额
    string tgt_ccy;          // 目标币种
    string category;         // 订单种类
    string u_time;           // 订单更新时间
    string c_time;           // 订单创建时间
    string req_id;           // 用户请求ID
    string amend_result;     // 修改订单的结果
    string code;             // 错误码
    string msg;              // 错误信息
    string reduce_only;      // 是否只减仓
    string quick_mgn_type;   // 一键借币类型
    string algo_cl_ord_id;   // 客户自定义策略订单ID
    string algo_id;          // 策略订单ID
    string attach_algo_ords; // 附带止盈止损信息
    string linked_ord;       // 下单附带止盈止损时，客户自定义的策略订单ID
    string stp_id;           // 自成交保护ID
    string stp_mode;         // 自成交保护模式
    // ... 更多字段
    
    // 自定义扩展字段
    Timestamp create_time_ms;
    Timestamp update_time_ms;
    Timestamp fill_time_ms;
    double sz_numeric;
    double px_numeric;
    double avg_px_numeric;
    double fee_numeric;
    double pnl_numeric;
};

// 成交明细（全字段）
struct TradeDetail {
    string inst_type;        // 产品类型
    string inst_id;          // 产品ID
    string trade_id;         // 成交ID
    string ord_id;           // 订单ID
    string cl_ord_id;        // 客户自定义订单ID
    string bill_id;          // 账单ID
    string tag;              // 订单标签
    string fill_px;          // 成交价格
    string fill_sz;          // 成交数量
    string side;             // 订单方向
    string pos_side;         // 持仓方向
    string exec_type;        // 流动性方向 T taker M maker
    string fee_ccy;          // 交易手续费币种
    string fee;              // 手续费金额
    string ts;               // 成交时间
    
    // 自定义扩展字段
    Timestamp fill_time_ms;
    double fill_px_numeric;
    double fill_sz_numeric;
    double fee_numeric;
};

// 账单记录（全字段）
struct BillDetail {
    string inst_type;        // 产品类型
    string inst_id;          // 产品ID
    string ccy;              // 账单币种
    string bill_id;          // 账单ID
    string type;             // 账单类型
    string sub_type;         // 账单子类型
    string ts;               // 账单创建时间
    string bal_chg;          // 账户层面的余额变动数量
    string pos_bal_chg;      // 仓位层面的余额变动数量
    string bal;              // 账户层面的余额数量
    string pos_bal;          // 仓位层面的余额数量
    string sz;               // 数量
    string px;               // 价格
    string pnl;              // 收益
    string fee;              // 手续费
    string ord_id;           // 订单ID
    string fill_mark_vol;    // 成交时标记波动率（仅适用于期权）
    string fill_forward_px;  // 成交时远期价格（仅适用于期权）
    string fill_mark_px;     // 成交时标记价格
    string fill_px;          // 成交价格
    string fill_time;        // 成交时间
    string from;             // 转出账户
    string to;               // 转入账户
    string notes;            // 备注
    string interest;         // 利息
    string trade_id;         // 成交ID
    // ... 更多字段
    
    // 自定义扩展字段
    Timestamp timestamp_ms;
    double bal_chg_numeric;
    double fee_numeric;
    double pnl_numeric;
};
```

---

### 模块M2：OKX REST API完整封装

**目的**: 封装所有OKX REST API，支持全字段获取

#### 核心类设计

```cpp
// okx_api_client.h
class OKXAPIClient {
public:
    // 初始化
    bool Initialize(const string& api_key, 
                    const string& secret_key,
                    const string& passphrase,
                    bool is_simulation = false);
    
    // ==================== 账户 API ====================
    
    // 获取账户配置
    Result<AccountConfig> GetAccountConfig();
    
    // 获取账户余额（全字段）
    Result<vector<AccountBalance>> GetBalance(const string& ccy = "");
    
    // 获取持仓信息（全字段）
    Result<vector<PositionDetail>> GetPositions(
        const string& inst_type = "",
        const string& inst_id = ""
    );
    
    // 获取持仓历史
    Result<vector<PositionDetail>> GetPositionsHistory(
        const string& inst_type = "",
        const string& inst_id = "",
        const string& mg_mode = "",
        const string& type = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取账户和持仓风险
    Result<json> GetAccountPositionRisk(const string& inst_type = "");
    
    // 获取账单流水（全字段）
    Result<vector<BillDetail>> GetBills(
        const string& inst_type = "",
        const string& ccy = "",
        const string& mg_mode = "",
        const string& ct_type = "",
        const string& type = "",
        const string& sub_type = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取账单流水（3个月内）
    Result<vector<BillDetail>> GetBillsArchive(
        const string& inst_type = "",
        const string& ccy = "",
        const string& mg_mode = "",
        const string& ct_type = "",
        const string& type = "",
        const string& sub_type = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // ==================== 交易 API ====================
    
    // 下单
    Result<OrderDetail> PlaceOrder(const OrderRequest& request);
    
    // 批量下单
    Result<vector<OrderDetail>> PlaceMultipleOrders(
        const vector<OrderRequest>& requests
    );
    
    // 撤单
    Result<OrderDetail> CancelOrder(
        const string& inst_id,
        const string& ord_id = "",
        const string& cl_ord_id = ""
    );
    
    // 批量撤单
    Result<vector<OrderDetail>> CancelMultipleOrders(
        const vector<CancelRequest>& requests
    );
    
    // 修改订单
    Result<OrderDetail> AmendOrder(const AmendRequest& request);
    
    // 批量修改订单
    Result<vector<OrderDetail>> AmendMultipleOrders(
        const vector<AmendRequest>& requests
    );
    
    // 市价平仓
    Result<OrderDetail> ClosePosition(
        const string& inst_id,
        const string& mg_mode,
        const string& pos_side = "",
        const string& ccy = "",
        bool auto_cancel = false
    );
    
    // 获取订单信息（全字段）
    Result<OrderDetail> GetOrder(
        const string& inst_id,
        const string& ord_id = "",
        const string& cl_ord_id = ""
    );
    
    // 获取未成交订单列表
    Result<vector<OrderDetail>> GetPendingOrders(
        const string& inst_type = "",
        const string& inst_id = "",
        const string& uly = "",
        const string& ord_type = "",
        const string& state = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取历史订单记录（近7天）
    Result<vector<OrderDetail>> GetOrderHistory(
        const string& inst_type,
        const string& inst_id = "",
        const string& uly = "",
        const string& ord_type = "",
        const string& state = "",
        const string& category = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取历史订单记录（近3个月）
    Result<vector<OrderDetail>> GetOrderHistoryArchive(
        const string& inst_type,
        const string& inst_id = "",
        const string& uly = "",
        const string& ord_type = "",
        const string& state = "",
        const string& category = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取成交明细（全字段）
    Result<vector<TradeDetail>> GetFills(
        const string& inst_type = "",
        const string& inst_id = "",
        const string& ord_id = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取成交明细（近3个月）
    Result<vector<TradeDetail>> GetFillsHistory(
        const string& inst_type,
        const string& inst_id = "",
        const string& ord_id = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // ==================== 行情 API ====================
    
    // 获取产品信息
    Result<vector<InstrumentInfo>> GetInstruments(
        const string& inst_type,
        const string& uly = "",
        const string& inst_id = ""
    );
    
    // 获取交割和行权记录
    Result<json> GetDeliveryExerciseHistory(
        const string& inst_type,
        const string& uly = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取持仓总量
    Result<json> GetOpenInterest(const string& inst_type, const string& uly = "");
    
    // 获取资金费率
    Result<vector<FundingRate>> GetFundingRate(const string& inst_id);
    
    // 获取资金费率历史
    Result<vector<FundingRate>> GetFundingRateHistory(
        const string& inst_id,
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取限价
    Result<json> GetPriceLimit(const string& inst_id);
    
    // 获取期权定价
    Result<json> GetOptionSummary(
        const string& uly,
        const string& exp_time = ""
    );
    
    // 获取预估交割/行权价格
    Result<json> GetEstimatedPrice(const string& inst_id);
    
    // 获取折价率
    Result<json> GetDiscountRateInterestFreeQuota(
        const string& ccy = "",
        int discount_lv = 1
    );
    
    // 获取系统时间
    Result<Timestamp> GetSystemTime();
    
    // 获取标记价格
    Result<json> GetMarkPrice(
        const string& inst_type,
        const string& uly = "",
        const string& inst_id = ""
    );
    
    // 获取指数行情
    Result<json> GetIndexTickers(
        const string& quote_ccy = "",
        const string& inst_id = ""
    );
    
    // 获取指数K线
    Result<vector<Candle>> GetIndexCandles(
        const string& inst_id,
        const string& bar = "1m",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取指数历史K线
    Result<vector<Candle>> GetIndexCandlesHistory(
        const string& inst_id,
        const string& bar = "1m",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取标记价格K线
    Result<vector<Candle>> GetMarkPriceCandles(
        const string& inst_id,
        const string& bar = "1m",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取标记价格历史K线
    Result<vector<Candle>> GetMarkPriceCandlesHistory(
        const string& inst_id,
        const string& bar = "1m",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取交易产品公共成交数据
    Result<vector<PublicTrade>> GetPublicTrades(
        const string& inst_id,
        int limit = 100
    );
    
    // 获取交易产品历史公共成交数据
    Result<vector<PublicTrade>> GetPublicTradesHistory(
        const string& inst_id,
        const string& type = "1",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // 获取24小时成交量
    Result<json> Get24HVolume();
    
    // ==================== WebSocket相关（用于REST补充）====================
    
    // Ticker
    Result<Ticker> GetTicker(const string& inst_id);
    
    // 深度
    Result<Depth> GetOrderBook(const string& inst_id, int sz = 5);
    
    // K线
    Result<vector<Candle>> GetCandles(
        const string& inst_id,
        const string& bar = "1m",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // K线历史（最近3个月）
    Result<vector<Candle>> GetCandlesHistory(
        const string& inst_id,
        const string& bar = "1m",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );

private:
    // 签名算法
    string GenerateSignature(
        const string& timestamp,
        const string& method,
        const string& request_path,
        const string& body
    );
    
    // HTTP请求
    Result<json> HttpRequest(
        const string& method,
        const string& endpoint,
        const json& params = json(),
        bool need_sign = true
    );
    
    // API密钥
    string api_key_;
    string secret_key_;
    string passphrase_;
    bool is_simulation_;
    
    // Base URL
    string base_url_;
    
    // 限流器
    RateLimiter rate_limiter_;
    
    // HTTP客户端
    unique_ptr<HttpClient> http_client_;
};
```

#### 为什么这样设计？

1. **全字段支持**: 所有API返回的字段都完整映射
2. **类型安全**: 使用强类型而不是裸JSON
3. **易于扩展**: 新增API只需添加一个函数
4. **错误处理**: Result<T>统一错误处理
5. **平台无关**: 更换平台只需实现相同接口

---

### 模块M3：WebSocket客户端

**目的**: 实时接收OKX推送数据

#### 核心类设计

```cpp
// websocket_client.h
class WebSocketClient {
public:
    // 初始化
    bool Initialize(const string& url, bool is_private = false);
    
    // 登录（私有频道需要）
    bool Login(const string& api_key,
               const string& secret_key,
               const string& passphrase);
    
    // 订阅公共频道
    bool SubscribeTicker(const string& inst_id,
                         function<void(const Ticker&)> callback);
    
    bool SubscribeOrderBook(const string& inst_id,
                            const string& channel, // books/books5/bbo-tbt等
                            function<void(const Depth&)> callback);
    
    bool SubscribeCandles(const string& inst_id,
                          const string& bar,
                          function<void(const Candle&)> callback);
    
    bool SubscribeTrades(const string& inst_id,
                         function<void(const PublicTrade&)> callback);
    
    bool SubscribeFundingRate(const string& inst_id,
                              function<void(const FundingRate&)> callback);
    
    bool SubscribeIndexTickers(const string& inst_id,
                               function<void(const json&)> callback);
    
    bool SubscribeMarkPrice(const string& inst_id,
                            function<void(const json&)> callback);
    
    // 订阅私有频道
    bool SubscribeAccount(function<void(const vector<AccountBalance>&)> callback);
    
    bool SubscribePositions(const string& inst_type,
                            const string& inst_id,
                            function<void(const vector<PositionDetail>&)> callback);
    
    bool SubscribeOrders(const string& inst_type,
                         const string& inst_id,
                         function<void(const vector<OrderDetail>&)> callback);
    
    bool SubscribeAlgoOrders(const string& inst_type,
                             const string& inst_id,
                             function<void(const json&)> callback);
    
    // 取消订阅
    bool Unsubscribe(const string& channel, const string& inst_id = "");
    
    bool UnsubscribeAll();
    
    // 连接管理
    bool Connect();
    bool Disconnect();
    bool IsConnected() const;
    
    // 自动重连
    void EnableAutoReconnect(bool enable, int delay_seconds = 5);
    
    // 心跳
    void EnablePing(bool enable, int interval_seconds = 20);
    
    // 错误回调
    void SetErrorCallback(function<void(const string&)> callback);
    
    // 连接状态回调
    void SetConnectionCallback(function<void(bool)> callback);

private:
    // 消息处理
    void OnMessage(const string& message);
    
    // 心跳
    void SendPing();
    
    // 重连
    void Reconnect();
    
    // WebSocket连接
    unique_ptr<WebSocket> ws_;
    
    // 订阅管理
    map<string, vector<function<void(const json&)>>> subscribers_;
    
    // 认证信息
    string api_key_;
    string secret_key_;
    string passphrase_;
    bool is_private_;
    bool is_logged_in_;
    
    // 重连控制
    bool auto_reconnect_;
    int reconnect_delay_;
    int reconnect_attempts_;
    
    // 心跳控制
    bool enable_ping_;
    int ping_interval_;
    Timestamp last_ping_time_;
    
    // 回调
    function<void(const string&)> error_callback_;
    function<void(bool)> connection_callback_;
    
    // 线程安全
    mutex mutex_;
    thread ping_thread_;
};
```

---

### 模块M4：SQLite数据库（14张表，100+字段）

**目的**: 完整存储所有OKX数据

#### 数据库Schema

```sql
-- 1. 账户配置表
CREATE TABLE account_config (
    uid TEXT PRIMARY KEY,
    acct_lv TEXT,
    pos_mode TEXT,
    auto_loan INTEGER,
    greeks_type TEXT,
    level TEXT,
    level_tmp TEXT,
    ct_iso_mode TEXT,
    mg_iso_mode TEXT,
    spot_offset_type TEXT,
    role_type TEXT,
    trade_role TEXT,
    update_time INTEGER
);

-- 2. 账户余额表
CREATE TABLE account_balance (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ccy TEXT NOT NULL,
    eq TEXT,
    cash_bal TEXT,
    upl TEXT,
    avail_eq TEXT,
    dis_eq TEXT,
    avail_bal TEXT,
    frozen_bal TEXT,
    ord_frozen TEXT,
    liab TEXT,
    upl_liab TEXT,
    cross_liab TEXT,
    iso_liab TEXT,
    margin_ratio TEXT,
    interest TEXT,
    twap TEXT,
    max_loan TEXT,
    eq_usd TEXT,
    notional_lever TEXT,
    stgy_eq TEXT,
    iso_upl TEXT,
    spot_in_use_amt TEXT,
    spot_iso_upl TEXT,
    update_time INTEGER,
    UNIQUE(ccy, update_time)
);

-- 3. 持仓表（全字段）
CREATE TABLE positions (
    pos_id TEXT PRIMARY KEY,
    inst_type TEXT,
    inst_id TEXT,
    pos_side TEXT,
    pos TEXT,
    avail_pos TEXT,
    avg_px TEXT,
    mark_px TEXT,
    upl TEXT,
    upl_ratio TEXT,
    upl_last_px TEXT,
    upl_ratio_last_px TEXT,
    inst_type_px TEXT,
    notional_usd TEXT,
    adl TEXT,
    imr TEXT,
    margin TEXT,
    margin_ratio TEXT,
    mmr TEXT,
    liab TEXT,
    liab_ccy TEXT,
    interest TEXT,
    trade_id TEXT,
    opt_val TEXT,
    pending_close_ord_liab_val TEXT,
    notional_ccy TEXT,
    liq_px TEXT,
    lever TEXT,
    delta_bs TEXT,
    delta_pa TEXT,
    gamma_bs TEXT,
    gamma_pa TEXT,
    theta_bs TEXT,
    theta_pa TEXT,
    vega_bs TEXT,
    vega_pa TEXT,
    spot_in_use_amt TEXT,
    spot_in_use_ccy TEXT,
    cl_spot_in_use_amt TEXT,
    max_spot_in_use_amt TEXT,
    biz_ref_id TEXT,
    biz_ref_type TEXT,
    base_bal TEXT,
    quote_bal TEXT,
    base_borrowed TEXT,
    base_interest TEXT,
    quote_borrowed TEXT,
    quote_interest TEXT,
    pos_ccy TEXT,
    fee TEXT,
    funding_fee TEXT,
    liq_penalty TEXT,
    realized_pnl TEXT,
    pnl TEXT,
    close_order_algo TEXT,
    c_time TEXT,
    u_time TEXT,
    px_usd TEXT,
    create_time_ms INTEGER,
    update_time_ms INTEGER
);

-- 4. 订单表（全字段）
CREATE TABLE orders (
    ord_id TEXT PRIMARY KEY,
    inst_type TEXT,
    inst_id TEXT,
    ccy TEXT,
    cl_ord_id TEXT,
    tag TEXT,
    px TEXT,
    sz TEXT,
    pnl TEXT,
    ord_type TEXT,
    side TEXT,
    pos_side TEXT,
    td_mode TEXT,
    acc_fill_sz TEXT,
    fill_px TEXT,
    trade_id TEXT,
    fill_sz TEXT,
    fill_time TEXT,
    state TEXT,
    avg_px TEXT,
    lever TEXT,
    tp_trigger_px TEXT,
    tp_trigger_px_type TEXT,
    tp_ord_px TEXT,
    sl_trigger_px TEXT,
    sl_trigger_px_type TEXT,
    sl_ord_px TEXT,
    fee_ccy TEXT,
    fee TEXT,
    rebate_ccy TEXT,
    rebate TEXT,
    tgt_ccy TEXT,
    category TEXT,
    u_time TEXT,
    c_time TEXT,
    req_id TEXT,
    amend_result TEXT,
    code TEXT,
    msg TEXT,
    reduce_only TEXT,
    quick_mgn_type TEXT,
    algo_cl_ord_id TEXT,
    algo_id TEXT,
    attach_algo_ords TEXT,
    linked_ord TEXT,
    stp_id TEXT,
    stp_mode TEXT,
    create_time_ms INTEGER,
    update_time_ms INTEGER,
    fill_time_ms INTEGER
);
CREATE INDEX idx_orders_inst_id ON orders(inst_id);
CREATE INDEX idx_orders_state ON orders(state);
CREATE INDEX idx_orders_create_time ON orders(create_time_ms);

-- 5. 成交记录表
CREATE TABLE trades (
    trade_id TEXT PRIMARY KEY,
    inst_type TEXT,
    inst_id TEXT,
    ord_id TEXT,
    cl_ord_id TEXT,
    bill_id TEXT,
    tag TEXT,
    fill_px TEXT,
    fill_sz TEXT,
    side TEXT,
    pos_side TEXT,
    exec_type TEXT,
    fee_ccy TEXT,
    fee TEXT,
    ts TEXT,
    fill_time_ms INTEGER
);
CREATE INDEX idx_trades_ord_id ON trades(ord_id);
CREATE INDEX idx_trades_inst_id ON trades(inst_id);
CREATE INDEX idx_trades_fill_time ON trades(fill_time_ms);

-- 6. 账单表
CREATE TABLE bills (
    bill_id TEXT PRIMARY KEY,
    inst_type TEXT,
    inst_id TEXT,
    ccy TEXT,
    type TEXT,
    sub_type TEXT,
    ts TEXT,
    bal_chg TEXT,
    pos_bal_chg TEXT,
    bal TEXT,
    pos_bal TEXT,
    sz TEXT,
    px TEXT,
    pnl TEXT,
    fee TEXT,
    ord_id TEXT,
    fill_mark_vol TEXT,
    fill_forward_px TEXT,
    fill_mark_px TEXT,
    fill_px TEXT,
    fill_time TEXT,
    from_account TEXT,
    to_account TEXT,
    notes TEXT,
    interest TEXT,
    trade_id TEXT,
    timestamp_ms INTEGER
);
CREATE INDEX idx_bills_ord_id ON bills(ord_id);
CREATE INDEX idx_bills_type ON bills(type);
CREATE INDEX idx_bills_timestamp ON bills(timestamp_ms);

-- 7. 资金费率表
CREATE TABLE funding_rates (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    inst_id TEXT,
    funding_rate TEXT,
    funding_time TEXT,
    next_funding_rate TEXT,
    next_funding_time TEXT,
    min_funding_rate TEXT,
    max_funding_rate TEXT,
    settlement_time TEXT,
    funding_time_ms INTEGER,
    next_funding_time_ms INTEGER,
    UNIQUE(inst_id, funding_time_ms)
);

-- 8. Ticker缓存表
CREATE TABLE ticker_cache (
    inst_id TEXT PRIMARY KEY,
    last TEXT,
    last_sz TEXT,
    ask_px TEXT,
    ask_sz TEXT,
    bid_px TEXT,
    bid_sz TEXT,
    open_24h TEXT,
    high_24h TEXT,
    low_24h TEXT,
    vol_ccy_24h TEXT,
    vol_24h TEXT,
    sod_utc0 TEXT,
    sod_utc8 TEXT,
    ts TEXT,
    update_time_ms INTEGER
);

-- 9. 深度缓存表
CREATE TABLE depth_cache (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    inst_id TEXT,
    level INTEGER,
    side TEXT, -- bid/ask
    price TEXT,
    size TEXT,
    timestamp_ms INTEGER,
    UNIQUE(inst_id, level, side)
);

-- 10. K线缓存表
CREATE TABLE candle_cache (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    inst_id TEXT,
    bar TEXT,
    ts TEXT,
    open TEXT,
    high TEXT,
    low TEXT,
    close TEXT,
    vol TEXT,
    vol_ccy TEXT,
    vol_ccy_quote TEXT,
    confirm TEXT,
    timestamp_ms INTEGER,
    UNIQUE(inst_id, bar, timestamp_ms)
);

-- 11. 产品信息表
CREATE TABLE instruments (
    inst_id TEXT PRIMARY KEY,
    inst_type TEXT,
    uly TEXT,
    category TEXT,
    base_ccy TEXT,
    quote_ccy TEXT,
    settle_ccy TEXT,
    ct_val TEXT,
    ct_mult TEXT,
    ct_val_ccy TEXT,
    opt_type TEXT,
    stk TEXT,
    list_time TEXT,
    exp_time TEXT,
    lever TEXT,
    tick_sz TEXT,
    lot_sz TEXT,
    min_sz TEXT,
    ct_type TEXT,
    alias TEXT,
    state TEXT,
    max_lmr TEXT,
    max_mkt_sz TEXT,
    max_twap_sz TEXT,
    max_iceberg_sz TEXT,
    max_trigger_sz TEXT,
    max_stop_sz TEXT,
    update_time_ms INTEGER
);

-- 12. 扩展数据KV表（用于存储自定义字段）
CREATE TABLE extension_data (
    key TEXT PRIMARY KEY,
    value TEXT,
    update_time INTEGER
);

-- 13. 系统配置表
CREATE TABLE system_config (
    key TEXT PRIMARY KEY,
    value TEXT,
    update_time INTEGER
);

-- 14. 同步日志表
CREATE TABLE sync_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    module TEXT,
    action TEXT,
    status TEXT,
    message TEXT,
    timestamp INTEGER
);
```

#### DatabaseManager类设计

```cpp
// database_manager.h
class DatabaseManager {
public:
    bool Initialize(const string& db_path);
    
    // ==================== 账户 ====================
    bool SaveAccountConfig(const AccountConfig& config);
    Result<AccountConfig> GetAccountConfig();
    
    bool SaveAccountBalance(const vector<AccountBalance>& balances);
    Result<vector<AccountBalance>> GetAccountBalance(const string& ccy = "");
    
    // ==================== 持仓 ====================
    bool SavePosition(const PositionDetail& pos);
    bool SavePositions(const vector<PositionDetail>& positions);
    Result<vector<PositionDetail>> GetPositions(
        const string& inst_id = "",
        const string& inst_type = ""
    );
    Result<PositionDetail> GetPosition(const string& pos_id);
    
    // ==================== 订单 ====================
    bool SaveOrder(const OrderDetail& order);
    bool SaveOrders(const vector<OrderDetail>& orders);
    Result<vector<OrderDetail>> GetOrders(
        const string& inst_id = "",
        const string& state = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    Result<OrderDetail> GetOrder(const string& ord_id);
    
    // ==================== 成交 ====================
    bool SaveTrade(const TradeDetail& trade);
    bool SaveTrades(const vector<TradeDetail>& trades);
    Result<vector<TradeDetail>> GetTrades(
        const string& ord_id = "",
        const string& inst_id = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // ==================== 账单 ====================
    bool SaveBill(const BillDetail& bill);
    bool SaveBills(const vector<BillDetail>& bills);
    Result<vector<BillDetail>> GetBills(
        const string& type = "",
        const string& inst_id = "",
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // ==================== 资金费率 ====================
    bool SaveFundingRate(const FundingRate& rate);
    Result<vector<FundingRate>> GetFundingRates(
        const string& inst_id,
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // ==================== 缓存 ====================
    bool SaveTicker(const Ticker& ticker);
    Result<Ticker> GetTicker(const string& inst_id);
    
    bool SaveDepth(const Depth& depth);
    Result<Depth> GetDepth(const string& inst_id);
    
    bool SaveCandle(const Candle& candle);
    Result<vector<Candle>> GetCandles(
        const string& inst_id,
        const string& bar,
        Timestamp after = 0,
        Timestamp before = 0,
        int limit = 100
    );
    
    // ==================== 原始SQL查询 ====================
    Result<json> ExecuteSQL(const string& sql);
    
    // ==================== 扩展数据 ====================
    bool SetExtensionData(const string& key, const string& value);
    Result<string> GetExtensionData(const string& key);
    bool DeleteExtensionData(const string& key);
    
    // ==================== 系统 ====================
    bool Backup(const string& backup_path);
    bool Vacuum();
    int64_t GetDatabaseSize();
    
private:
    unique_ptr<SQLite::Database> db_;
    mutex mutex_;
};
```

---

### 模块M5：OKX Data Provider（数据聚合层）

**目的**: 统一管理REST API、WebSocket、数据库

```cpp
// okx_data_provider.h
class OKXDataProvider {
public:
    // 初始化
    bool Initialize(
        const string& api_key,
        const string& secret_key,
        const string& passphrase,
        const string& db_path
    );
    
    // 启动
    bool Start();
    
    // 停止
    void Stop();
    
    // ==================== 自动同步 ====================
    
    // 启用自动同步
    void EnableAutoSync(bool enable, int interval_seconds = 60);
    
    // 手动同步
    bool SyncAccount();
    bool SyncPositions();
    bool SyncOrders();
    bool SyncTrades();
    bool SyncBills();
    bool SyncAll();
    
    // ==================== 数据获取（优先从缓存）====================
    
    // 账户
    Result<AccountConfig> GetAccountConfig(bool force_api = false);
    Result<vector<AccountBalance>> GetBalance(const string& ccy = "", bool force_api = false);
    
    // 持仓
    Result<vector<PositionDetail>> GetPositions(
        const string& inst_type = "",
        const string& inst_id = "",
        bool force_api = false
    );
    
    // 订单
    Result<vector<OrderDetail>> GetOrders(
        const string& inst_id = "",
        const string& state = "",
        bool force_api = false
    );
    
    // 成交
    Result<vector<TradeDetail>> GetTrades(
        const string& ord_id = "",
        const string& inst_id = "",
        bool force_api = false
    );
    
    // 账单
    Result<vector<BillDetail>> GetBills(
        const string& type = "",
        const string& inst_id = "",
        bool force_api = false
    );
    
    // 行情
    Result<Ticker> GetTicker(const string& inst_id, bool force_api = false);
    Result<Depth> GetDepth(const string& inst_id, bool force_api = false);
    Result<vector<Candle>> GetCandles(
        const string& inst_id,
        const string& bar = "1m",
        int limit = 100,
        bool force_api = false
    );
    
    // ==================== WebSocket订阅 ====================
    
    bool SubscribeTicker(const string& inst_id);
    bool SubscribeDepth(const string& inst_id);
    bool SubscribeOrders();
    bool SubscribePositions();
    bool SubscribeAccount();
    
    // ==================== 交易 ====================
    
    Result<OrderDetail> PlaceOrder(const OrderRequest& request);
    Result<OrderDetail> CancelOrder(const string& inst_id, const string& ord_id);
    
    // ==================== 统计 ====================
    
    struct Statistics {
        int total_orders;
        int filled_orders;
        int canceled_orders;
        double total_pnl;
        double total_fee;
        int total_trades;
        Timestamp last_sync_time;
    };
    
    Statistics GetStatistics();

private:
    // API客户端
    unique_ptr<OKXAPIClient> api_client_;
    
    // WebSocket客户端
    unique_ptr<WebSocketClient> ws_public_;
    unique_ptr<WebSocketClient> ws_private_;
    
    // 数据库
    unique_ptr<DatabaseManager> db_;
    
    // 自动同步
    bool auto_sync_enabled_;
    int sync_interval_;
    thread sync_thread_;
    atomic<bool> running_;
    
    // 缓存
    Cache<string, Ticker> ticker_cache_;
    Cache<string, Depth> depth_cache_;
    
    // 线程安全
    mutex mutex_;
};
```

---

### 模块M6：DLL导出接口（60+函数）

**目的**: 将所有功能导出供MT5调用

```cpp
// dllmain.cpp

extern "C" {

// ==================== 初始化/关闭 ====================

__declspec(dllexport) int __stdcall OKX_Initialize(
    const char* api_key,
    const char* secret_key,
    const char* passphrase,
    const char* db_path
);

__declspec(dllexport) int __stdcall OKX_Start();

__declspec(dllexport) void __stdcall OKX_Stop();

__declspec(dllexport) void __stdcall OKX_Shutdown();

__declspec(dllexport) int __stdcall OKX_GetSystemStatus(char* out_json, int buffer_size);

// ==================== 账户 ====================

__declspec(dllexport) int __stdcall OKX_GetAccountConfig(char* out_json, int buffer_size);

__declspec(dllexport) int __stdcall OKX_GetBalance(
    const char* ccy,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetBalanceUSD(double* out_value);

// ==================== 持仓 ====================

__declspec(dllexport) int __stdcall OKX_GetPositions(
    const char* inst_type,
    const char* inst_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetPosition(
    const char* inst_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetPositionCount(int* out_count);

__declspec(dllexport) int __stdcall OKX_GetTotalPnL(double* out_pnl);

// ==================== 订单 ====================

__declspec(dllexport) int __stdcall OKX_PlaceOrder(
    const char* request_json,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_CancelOrder(
    const char* inst_id,
    const char* ord_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetOrder(
    const char* ord_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetPendingOrders(
    const char* inst_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetOrderHistory(
    const char* inst_id,
    long after,
    long before,
    int limit,
    char* out_json,
    int buffer_size
);

// ==================== 成交 ====================

__declspec(dllexport) int __stdcall OKX_GetTrades(
    const char* ord_id,
    const char* inst_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetTradeCount(
    const char* inst_id,
    int* out_count
);

// ==================== 账单 ====================

__declspec(dllexport) int __stdcall OKX_GetBills(
    const char* type,
    const char* inst_id,
    long after,
    long before,
    int limit,
    char* out_json,
    int buffer_size
);

// ==================== 行情 ====================

__declspec(dllexport) int __stdcall OKX_GetTicker(
    const char* inst_id,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetTickerPrice(
    const char* inst_id,
    double* bid,
    double* ask
);

__declspec(dllexport) int __stdcall OKX_GetDepth(
    const char* inst_id,
    int levels,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetCandles(
    const char* inst_id,
    const char* bar,
    long after,
    long before,
    int limit,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_GetFundingRate(
    const char* inst_id,
    char* out_json,
    int buffer_size
);

// ==================== WebSocket订阅 ====================

__declspec(dllexport) int __stdcall OKX_SubscribeTicker(const char* inst_id);

__declspec(dllexport) int __stdcall OKX_SubscribeDepth(const char* inst_id);

__declspec(dllexport) int __stdcall OKX_SubscribeOrders();

__declspec(dllexport) int __stdcall OKX_SubscribePositions();

__declspec(dllexport) int __stdcall OKX_SubscribeAccount();

// ==================== 本地数据库查询 ====================

__declspec(dllexport) int __stdcall OKX_QueryLocalOrders(
    const char* filter_json,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_QueryLocalTrades(
    const char* filter_json,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_QueryLocalBills(
    const char* filter_json,
    char* out_json,
    int buffer_size
);

__declspec(dllexport) int __stdcall OKX_ExecuteSQL(
    const char* sql,
    char* out_json,
    int buffer_size
);

// ==================== 扩展数据 ====================

__declspec(dllexport) int __stdcall OKX_SetExtensionData(
    const char* key,
    const char* value
);

__declspec(dllexport) int __stdcall OKX_GetExtensionData(
    const char* key,
    char* out_value,
    int buffer_size
);

// ==================== 同步 ====================

__declspec(dllexport) int __stdcall OKX_SyncAll();

__declspec(dllexport) int __stdcall OKX_SyncAccount();

__declspec(dllexport) int __stdcall OKX_SyncPositions();

__declspec(dllexport) int __stdcall OKX_SyncOrders();

__declspec(dllexport) int __stdcall OKX_EnableAutoSync(int enable, int interval_seconds);

// ==================== 统计 ====================

__declspec(dllexport) int __stdcall OKX_GetStatistics(char* out_json, int buffer_size);

} // extern "C"
```

---

### 模块M7：API验证工具（MT5 EA）

**目的**: 完整展示所有API数据，验证功能

#### EA设计

```mql5
//+------------------------------------------------------------------+
//|                                         OKX_API_Validator.mq5 |
//+------------------------------------------------------------------+

#import "okx_bridge.dll"
// 导入所有60+个函数
int OKX_Initialize(string api_key, string secret_key, string passphrase, string db_path);
int OKX_Start();
void OKX_Stop();
int OKX_GetAccountConfig(string &out_json, int buffer_size);
int OKX_GetBalance(string ccy, string &out_json, int buffer_size);
int OKX_GetPositions(string inst_type, string inst_id, string &out_json, int buffer_size);
int OKX_GetPendingOrders(string inst_id, string &out_json, int buffer_size);
int OKX_GetTrades(string ord_id, string inst_id, string &out_json, int buffer_size);
int OKX_GetBills(string type, string inst_id, long after, long before, int limit, string &out_json, int buffer_size);
int OKX_GetTicker(string inst_id, string &out_json, int buffer_size);
int OKX_GetDepth(string inst_id, int levels, string &out_json, int buffer_size);
int OKX_GetCandles(string inst_id, string bar, long after, long before, int limit, string &out_json, int buffer_size);
int OKX_GetFundingRate(string inst_id, string &out_json, int buffer_size);
int OKX_GetStatistics(string &out_json, int buffer_size);
int OKX_ExecuteSQL(string sql, string &out_json, int buffer_size);
void OKX_Shutdown();
#import

// 输入参数
input string API_KEY = "";
input string SECRET_KEY = "";
input string PASSPHRASE = "";
input string INST_ID = "XAUT-USDT-SWAP";

// 全局变量
bool g_initialized = false;
bool g_started = false;

// 面板对象
string g_panel_prefix = "OKX_";

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit() {
    // 1. 初始化DLL
    Print("正在初始化OKX DLL...");
    char buf[1024];
    if (OKX_Initialize(API_KEY, SECRET_KEY, PASSPHRASE, "okx_data.db") != 0) {
        Alert("DLL初始化失败！");
        return INIT_FAILED;
    }
    g_initialized = true;
    Print("DLL初始化成功！");
    
    // 2. 启动数据提供器
    Print("正在启动数据提供器...");
    if (OKX_Start() != 0) {
        Alert("数据提供器启动失败！");
        return INIT_FAILED;
    }
    g_started = true;
    Print("数据提供器启动成功！");
    
    // 3. 创建验证面板
    CreateValidationPanel();
    
    // 4. 启动定时器（每秒更新一次）
    EventSetTimer(1);
    
    return INIT_SUCCEEDED;
}

//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason) {
    EventKillTimer();
    
    if (g_started) {
        OKX_Stop();
    }
    
    if (g_initialized) {
        OKX_Shutdown();
    }
    
    DeleteAllPanelObjects();
}

//+------------------------------------------------------------------+
//| Timer function                                                   |
//+------------------------------------------------------------------+
void OnTimer() {
    // 更新所有面板数据
    UpdateAccountPanel();
    UpdatePositionPanel();
    UpdateOrderPanel();
    UpdateTradePanel();
    UpdateBillPanel();
    UpdateMarketDataPanel();
    UpdateStatisticsPanel();
}

//+------------------------------------------------------------------+
//| 创建验证面板                                                     |
//+------------------------------------------------------------------+
void CreateValidationPanel() {
    int x = 10;
    int y = 30;
    int width = 800;
    int height = 600;
    
    // 创建背景
    CreateRectangle(g_panel_prefix + "BG", x, y, width, height, clrWhiteSmoke);
    
    // 创建标题
    CreateLabel(g_panel_prefix + "Title", x + 10, y + 10, "OKX API完整验证工具", 14, clrBlack);
    
    // 创建7个子面板
    int panel_height = 70;
    int panel_y = y + 40;
    
    CreateSubPanel("Account", x, panel_y, width, panel_height, "账户信息");
    panel_y += panel_height + 10;
    
    CreateSubPanel("Position", x, panel_y, width, panel_height, "持仓信息");
    panel_y += panel_height + 10;
    
    CreateSubPanel("Order", x, panel_y, width, panel_height, "订单信息");
    panel_y += panel_height + 10;
    
    CreateSubPanel("Trade", x, panel_y, width, panel_height, "成交记录");
    panel_y += panel_height + 10;
    
    CreateSubPanel("Bill", x, panel_y, width, panel_height, "账单记录");
    panel_y += panel_height + 10;
    
    CreateSubPanel("Market", x, panel_y, width, panel_height, "行情数据");
    panel_y += panel_height + 10;
    
    CreateSubPanel("Stats", x, panel_y, width, panel_height, "统计信息");
}

//+------------------------------------------------------------------+
//| 创建子面板                                                       |
//+------------------------------------------------------------------+
void CreateSubPanel(string name, int x, int y, int width, int height, string title) {
    string prefix = g_panel_prefix + name + "_";
    
    // 背景
    CreateRectangle(prefix + "BG", x + 5, y, width - 10, height, clrLightGray);
    
    // 标题
    CreateLabel(prefix + "Title", x + 15, y + 5, title, 12, clrBlue);
    
    // 内容区域（将在Update函数中填充）
    CreateLabel(prefix + "Content", x + 15, y + 25, "正在加载...", 10, clrBlack);
}

//+------------------------------------------------------------------+
//| 更新账户面板                                                     |
//+------------------------------------------------------------------+
void UpdateAccountPanel() {
    char buf[65536];
    
    // 获取账户配置
    if (OKX_GetAccountConfig(buf, sizeof(buf)) == 0) {
        json config = JsonParse(buf);
        string text = StringFormat(
            "账户等级: %s | 持仓模式: %s | 自动借币: %s",
            config["level"].ToString(),
            config["pos_mode"].ToString(),
            config["auto_loan"].ToBool() ? "开启" : "关闭"
        );
        UpdateLabelText(g_panel_prefix + "Account_Content", text);
    }
    
    // 获取余额
    if (OKX_GetBalance("", buf, sizeof(buf)) == 0) {
        json balances = JsonParse(buf);
        double total_eq_usd = 0;
        
        for (int i = 0; i < ArraySize(balances); i++) {
            total_eq_usd += balances[i]["eq_usd"].ToDouble();
        }
        
        string text2 = StringFormat(
            "账户权益: %.2f USD | 币种数量: %d",
            total_eq_usd,
            ArraySize(balances)
        );
        UpdateLabelText(g_panel_prefix + "Account_Content2", text2);
    }
}

//+------------------------------------------------------------------+
//| 更新持仓面板                                                     |
//+------------------------------------------------------------------+
void UpdatePositionPanel() {
    char buf[65536];
    
    if (OKX_GetPositions("", INST_ID, buf, sizeof(buf)) == 0) {
        json positions = JsonParse(buf);
        
        if (ArraySize(positions) > 0) {
            json pos = positions[0];
            
            string text = StringFormat(
                "产品: %s | 方向: %s | 数量: %s | 均价: %s | 标记价: %s | "
                "未实现盈亏: %s | 盈亏率: %s | 保证金率: %s | 杠杆: %s",
                pos["inst_id"].ToString(),
                pos["pos_side"].ToString(),
                pos["pos"].ToString(),
                pos["avg_px"].ToString(),
                pos["mark_px"].ToString(),
                pos["upl"].ToString(),
                pos["upl_ratio"].ToString(),
                pos["margin_ratio"].ToString(),
                pos["lever"].ToString()
            );
            UpdateLabelText(g_panel_prefix + "Position_Content", text);
        } else {
            UpdateLabelText(g_panel_prefix + "Position_Content", "无持仓");
        }
    }
}

//+------------------------------------------------------------------+
//| 更新订单面板                                                     |
//+------------------------------------------------------------------+
void UpdateOrderPanel() {
    char buf[65536];
    
    if (OKX_GetPendingOrders(INST_ID, buf, sizeof(buf)) == 0) {
        json orders = JsonParse(buf);
        
        string text = StringFormat("活跃订单数量: %d", ArraySize(orders));
        
        if (ArraySize(orders) > 0) {
            json order = orders[0];
            text += StringFormat(
                " | 最新订单: ID=%s, 方向=%s, 价格=%s, 数量=%s, 状态=%s",
                order["ord_id"].ToString(),
                order["side"].ToString(),
                order["px"].ToString(),
                order["sz"].ToString(),
                order["state"].ToString()
            );
        }
        
        UpdateLabelText(g_panel_prefix + "Order_Content", text);
    }
}

//+------------------------------------------------------------------+
//| 更新成交面板                                                     |
//+------------------------------------------------------------------+
void UpdateTradePanel() {
    char buf[65536];
    
    if (OKX_GetTrades("", INST_ID, buf, sizeof(buf)) == 0) {
        json trades = JsonParse(buf);
        
        string text = StringFormat("成交记录数量: %d", ArraySize(trades));
        
        if (ArraySize(trades) > 0) {
            json trade = trades[0];
            text += StringFormat(
                " | 最新成交: ID=%s, 价格=%s, 数量=%s, 手续费=%s",
                trade["trade_id"].ToString(),
                trade["fill_px"].ToString(),
                trade["fill_sz"].ToString(),
                trade["fee"].ToString()
            );
        }
        
        UpdateLabelText(g_panel_prefix + "Trade_Content", text);
    }
}

//+------------------------------------------------------------------+
//| 更新账单面板                                                     |
//+------------------------------------------------------------------+
void UpdateBillPanel() {
    char buf[65536];
    
    long now = TimeCurrent() * 1000;
    long day_ago = now - 24 * 3600 * 1000;
    
    if (OKX_GetBills("", INST_ID, day_ago, now, 100, buf, sizeof(buf)) == 0) {
        json bills = JsonParse(buf);
        
        string text = StringFormat("24小时账单数量: %d", ArraySize(bills));
        
        double total_fee = 0;
        for (int i = 0; i < ArraySize(bills); i++) {
            total_fee += bills[i]["fee"].ToDouble();
        }
        
        text += StringFormat(" | 总手续费: %.4f", total_fee);
        
        UpdateLabelText(g_panel_prefix + "Bill_Content", text);
    }
}

//+------------------------------------------------------------------+
//| 更新行情数据面板                                                 |
//+------------------------------------------------------------------+
void UpdateMarketDataPanel() {
    char buf[65536];
    
    // Ticker
    if (OKX_GetTicker(INST_ID, buf, sizeof(buf)) == 0) {
        json ticker = JsonParse(buf);
        
        string text = StringFormat(
            "Ticker: 最新价=%s | 买一=%s | 卖一=%s | 24H成交量=%s | 24H成交额=%s",
            ticker["last"].ToString(),
            ticker["bid_px"].ToString(),
            ticker["ask_px"].ToString(),
            ticker["vol_24h"].ToString(),
            ticker["vol_ccy_24h"].ToString()
        );
        UpdateLabelText(g_panel_prefix + "Market_Content", text);
    }
    
    // 深度
    if (OKX_GetDepth(INST_ID, 5, buf, sizeof(buf)) == 0) {
        json depth = JsonParse(buf);
        
        string text2 = "深度: ";
        for (int i = 0; i < 3 && i < ArraySize(depth["bids"]); i++) {
            text2 += StringFormat("买%d=%s@%s ", i+1, 
                depth["bids"][i][1].ToString(),
                depth["bids"][i][0].ToString()
            );
        }
        UpdateLabelText(g_panel_prefix + "Market_Content2", text2);
    }
    
    // 资金费率
    if (OKX_GetFundingRate(INST_ID, buf, sizeof(buf)) == 0) {
        json rate = JsonParse(buf);
        
        string text3 = StringFormat(
            "资金费率: 当前=%s | 下次=%s | 下次时间=%s",
            rate["funding_rate"].ToString(),
            rate["next_funding_rate"].ToString(),
            TimeToString(rate["next_funding_time"].ToDouble() / 1000)
        );
        UpdateLabelText(g_panel_prefix + "Market_Content3", text3);
    }
}

//+------------------------------------------------------------------+
//| 更新统计面板                                                     |
//+------------------------------------------------------------------+
void UpdateStatisticsPanel() {
    char buf[65536];
    
    if (OKX_GetStatistics(buf, sizeof(buf)) == 0) {
        json stats = JsonParse(buf);
        
        string text = StringFormat(
            "总订单: %d | 成交订单: %d | 取消订单: %d | "
            "总盈亏: %.2f | 总手续费: %.4f | 总成交: %d | "
            "最后同步: %s",
            (int)stats["total_orders"].ToDouble(),
            (int)stats["filled_orders"].ToDouble(),
            (int)stats["canceled_orders"].ToDouble(),
            stats["total_pnl"].ToDouble(),
            stats["total_fee"].ToDouble(),
            (int)stats["total_trades"].ToDouble(),
            TimeToString(stats["last_sync_time"].ToDouble() / 1000)
        );
        UpdateLabelText(g_panel_prefix + "Stats_Content", text);
    }
}

//+------------------------------------------------------------------+
//| 辅助函数：创建矩形                                               |
//+------------------------------------------------------------------+
void CreateRectangle(string name, int x, int y, int width, int height, color clr) {
    ObjectCreate(0, name, OBJ_RECTANGLE_LABEL, 0, 0, 0);
    ObjectSetInteger(0, name, OBJPROP_XDISTANCE, x);
    ObjectSetInteger(0, name, OBJPROP_YDISTANCE, y);
    ObjectSetInteger(0, name, OBJPROP_XSIZE, width);
    ObjectSetInteger(0, name, OBJPROP_YSIZE, height);
    ObjectSetInteger(0, name, OBJPROP_BGCOLOR, clr);
    ObjectSetInteger(0, name, OBJPROP_BORDER_TYPE, BORDER_FLAT);
    ObjectSetInteger(0, name, OBJPROP_CORNER, CORNER_LEFT_UPPER);
}

//+------------------------------------------------------------------+
//| 辅助函数：创建标签                                               |
//+------------------------------------------------------------------+
void CreateLabel(string name, int x, int y, string text, int font_size, color clr) {
    ObjectCreate(0, name, OBJ_LABEL, 0, 0, 0);
    ObjectSetInteger(0, name, OBJPROP_XDISTANCE, x);
    ObjectSetInteger(0, name, OBJPROP_YDISTANCE, y);
    ObjectSetString(0, name, OBJPROP_TEXT, text);
    ObjectSetInteger(0, name, OBJPROP_FONTSIZE, font_size);
    ObjectSetInteger(0, name, OBJPROP_COLOR, clr);
    ObjectSetInteger(0, name, OBJPROP_CORNER, CORNER_LEFT_UPPER);
}

//+------------------------------------------------------------------+
//| 辅助函数：更新标签文本                                           |
//+------------------------------------------------------------------+
void UpdateLabelText(string name, string text) {
    ObjectSetString(0, name, OBJPROP_TEXT, text);
}

//+------------------------------------------------------------------+
//| 辅助函数：删除所有面板对象                                       |
//+------------------------------------------------------------------+
void DeleteAllPanelObjects() {
    int total = ObjectsTotal(0);
    for (int i = total - 1; i >= 0; i--) {
        string name = ObjectName(0, i);
        if (StringFind(name, g_panel_prefix) == 0) {
            ObjectDelete(0, name);
        }
    }
}
```

---

## 🎯 开发计划

### Week 1: 数据基础（M1-M4）

#### Day 1-2: 数据结构 + REST API
- [ ] 完成OKX全字段数据结构定义
- [ ] 实现OKXAPIClient类
- [ ] 实现签名算法
- [ ] 单元测试（账户/持仓/订单API）

#### Day 3-4: WebSocket + 数据库
- [ ] 实现WebSocketClient类
- [ ] 实现自动重连和心跳
- [ ] 创建14张数据库表
- [ ] 实现DatabaseManager类
- [ ] 单元测试

#### Day 5: 数据聚合层
- [ ] 实现OKXDataProvider类
- [ ] 实现缓存机制
- [ ] 实现自动同步
- [ ] 集成测试

### Week 2: DLL导出 + 验证工具（M5-M7）

#### Day 1-2: DLL导出接口
- [ ] 实现60+个导出函数
- [ ] 完善错误处理
- [ ] 编译测试
- [ ] 性能测试

#### Day 3-4: MT5验证工具
- [ ] 创建验证EA
- [ ] 实现7个数据面板
- [ ] 测试所有API调用
- [ ] 压力测试

#### Day 5: 文档与打包
- [ ] API文档（Doxygen）
- [ ] 用户手册
- [ ] 配置文件模板
- [ ] 安装脚本

---

## ✅ 验收标准

### 功能完整性
- [ ] 所有OKX私有API字段都已映射
- [ ] 14张数据库表全部创建
- [ ] 60+个DLL导出函数正常工作
- [ ] WebSocket实时推送正常
- [ ] REST API调用正常
- [ ] 验证工具能展示所有数据

### 性能指标
- [ ] Tick处理延迟 < 5ms
- [ ] REST API调用延迟 < 100ms
- [ ] 数据库查询延迟 < 20ms
- [ ] 内存占用 < 150MB
- [ ] 24小时连续运行无崩溃

### 扩展性验证
- [ ] 新增币种无需重新编译
- [ ] 自定义SQL查询正常
- [ ] 配置文件热重载正常
- [ ] 更换API只需修改配置

---

## 📦 交付物清单

1. **源代码**
   - C++ DLL源码（完整注释）
   - MT5 EA验证工具
   - 配置文件模板

2. **编译文件**
   - okx_bridge.dll (64位)
   - OKX_API_Validator.ex5

3. **文档**
   - API完整文档（HTML）
   - 用户手册（PDF）
   - 开发者指南
   - 数据库Schema文档

4. **测试报告**
   - 单元测试报告
   - 集成测试报告
   - 性能测试报告
   - 24小时稳定性测试报告

---

**准备好开始了吗？让我知道您是否需要调整任何内容！** 🚀
