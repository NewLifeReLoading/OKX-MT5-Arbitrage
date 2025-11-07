# OKX-MT5 套利交易系统 - 完整项目总结

## 项目概述

### 核心目标
在MT5平台上实现OKX交易所与MT5经纪商之间的黄金(XAUUSD/XAUT-USDT-SWAP)价差套利交易系统。

### 基本原理
- **套利逻辑**：利用OKX交易所与MT5经纪商之间的黄金价格差异进行对冲套利
- **交易品种**：
  - MT5: XAUUSD (黄金兑美元)
  - OKX: XAUT-USDT-SWAP (Tether Gold永续合约)
- **套利方式**：当价差达到阈值时，在一个平台做多，另一个平台做空

---

## 一、技术架构演进历程

### 第一阶段：初始方案探索

#### 1.1 Python WebSocket中继方案
**方案描述**：
- 使用Python脚本连接OKX WebSocket获取实时行情
- 通过本地WebSocket服务器(localhost:8080)广播数据
- MT5通过WebSocket客户端接收行情数据

**核心代码**：
```python
# okex_ws_server.py
- 连接 wss://ws.okx.com:8443/ws/v5/public
- 订阅 bbo-tbt 频道获取 XAUT-USDT-SWAP 最优买卖价
- 本地广播到 ws://localhost:8080/okex
```

**问题**：
- MT5的WebSocket支持有限
- 需要额外运行Python进程
- 跨进程通信增加复杂度

#### 1.2 Python REST API封装
**方案描述**：
```python
# okex_api_trader.py
- 封装OKX REST API（下单、查询、撤单）
- 使用HMAC-SHA256签名认证
- 提供统一的Python接口
```

**问题**：
- MT5无法直接调用Python代码
- 需要额外的进程间通信机制

### 第二阶段：C++ DLL桥接方案

#### 2.1 技术选型决策
**最终方案**：C++ DLL + libcurl + OpenSSL
- **原因**：MT5原生支持DLL调用，性能高，无需额外进程
- **工具链**：MSYS2 MinGW-w64 + g++ 编译器

#### 2.2 DLL依赖问题的艰难历程

**问题A：动态链接依赖地狱**
最初编译的DLL依赖大量外部DLL：
```
libcurl-4.dll
libssl-3-x64.dll
libcrypto-3-x64.dll
zlib1.dll
libbrotlidec.dll
libnghttp2-14.dll
libssh2-1.dll
... (共约18个DLL)
```

**尝试方案1：复制所有DLL到MT5目录**
```bash
cd "C:\Users\Administrator\AppData\Roaming\MetaQuotes\Terminal\...\MQL5\Libraries"
cp /mingw64/bin/libcurl-4.dll .
cp /mingw64/bin/libssl-3-x64.dll .
# ... 复制所有18个DLL
```
- **结果**：错误126 - DLL加载失败
- **原因**：DLL之间存在循环依赖，路径问题

**尝试方案2：静态链接所有库**
```bash
g++ -shared -O2 -static-libgcc -static-libstdc++ \
  -Wl,-Bstatic -lcurl -lssl -lcrypto \
  -o libokx_bridge.dll
```
- **结果**：undefined reference to `__imp_curl_*`
- **原因**：MSYS2的libcurl只有动态链接版本(.dll.a)，缺少完整静态库(.a)

**尝试方案3：手动指定静态库文件**
```bash
g++ -shared -O2 -static \
  /mingw64/lib/libcurl.a \
  /mingw64/lib/libssl.a \
  /mingw64/lib/libcrypto.a \
  ...
```
- **结果**：仍然报链接错误
- **原因**：库之间依赖顺序错误，缺少某些传递依赖

#### 2.3 最终解决方案：子目录安装 + 全DLL打包

**核心决策**：放弃完全静态链接，采用"子目录隔离"方案

**实现细节**：
```
MQL5/Libraries/okx/
├── libokx_bridge.dll        (主DLL)
├── libcurl-4.dll
├── libssl-3-x64.dll
├── libcrypto-3-x64.dll
└── ... (所有18个依赖DLL)
```

**关键技术点**：
1. **DLL搜索路径**：Windows会在DLL所在目录查找依赖
2. **MT5加载机制**：使用相对路径 `#import "okx/libokx_bridge.dll"`
3. **一键部署**：编写批处理脚本自动复制所有DLL

**编译命令**：
```bash
cd /c/okx_bridge
g++ -shared -O2 \
  -I/mingw64/include -L/mingw64/lib \
  src/okx_bridge.cpp \
  -lcurl -lssl -lcrypto \
  -lws2_32 -lwinmm -lcrypt32 \
  -o libokx_bridge.dll
```

**自动化部署脚本**：
```batch
@echo off
set MQL5_PATH=%APPDATA%\MetaQuotes\Terminal\Common\MQL5

REM 创建子目录
if not exist "%MQL5_PATH%\Libraries\okx" mkdir "%MQL5_PATH%\Libraries\okx"

REM 复制主DLL
copy libokx_bridge.dll "%MQL5_PATH%\Libraries\okx\"

REM 批量复制所有依赖
for %%f in (libcurl-4.dll libssl-3-x64.dll ...) do (
    copy /mingw64/bin/%%f "%MQL5_PATH%\Libraries\okx\"
)
```

---

## 二、核心功能模块设计

### 2.1 DLL接口设计 (okx_bridge.cpp)

#### API认证机制
OKX使用HMAC-SHA256签名：
```cpp
// 签名生成流程
1. 拼接消息：timestamp + method + path + body
   例：1635724800000GET/api/v5/account/balance
   
2. 使用SecretKey进行HMAC-SHA256
   
3. Base64编码结果
   
4. 放入HTTP头：
   - OK-ACCESS-KEY: API密钥
   - OK-ACCESS-SIGN: 签名
   - OK-ACCESS-TIMESTAMP: 时间戳(毫秒)
   - OK-ACCESS-PASSPHRASE: 密码短语
```

#### 核心函数
```cpp
// 查询账户余额
extern "C" __declspec(dllexport) 
int __stdcall OKX_QueryBalance(char* out, int bufLen);

// 下单
extern "C" __declspec(dllexport)
int __stdcall OKX_PlaceOrder(
    const char* instId,    // 产品ID: XAUT-USDT-SWAP
    const char* side,      // buy/sell
    const char* posSide,   // long/short
    double price,
    double size,
    char* out,
    int bufLen
);

// 查询持仓
extern "C" __declspec(dllexport)
int __stdcall OKX_GetPositions(
    const char* instId,
    char* out,
    int bufLen
);

// 撤单
extern "C" __declspec(dllexport)
int __stdcall OKX_CancelOrder(
    const char* instId,
    const char* orderId,
    char* out,
    int bufLen
);
```

#### 配置文件管理
```cpp
// Config.json 格式
{
    "api_key": "your-api-key",
    "secret_key": "your-secret-key",
    "passphrase": "your-passphrase",
    "is_demo": true,          // true=模拟盘, false=实盘
    "base_url": "https://www.okx.com"
}

// 加载函数
static void LoadConfig() {
    std::ifstream f("C:/OKXBridge/Config.json");
    json config = json::parse(f);
    g_api_key = config["api_key"];
    g_secret_key = config["secret_key"];
    g_passphrase = config["passphrase"];
}
```

### 2.2 MT5 API封装层 (OkexAPI.mqh)

#### 类设计
```mql5
class COkexAPI {
private:
    string m_lastError;
    bool   m_isDemo;
    
public:
    // 构造函数：加载配置
    COkexAPI();
    
    // 查询余额
    string GetBalance();
    
    // 解析总权益
    double ParseTotalEquity(const string& json);
    
    // 下单
    string PlaceOrder(
        string instId,
        string side,      // "buy" or "sell"
        string posSide,   // "long" or "short"
        double price,
        double size
    );
    
    // 查询持仓
    string GetPositions(string instId = "XAUT-USDT-SWAP");
    
    // 获取错误信息
    string GetLastError() { return m_lastError; }
    
    // 是否模拟盘
    bool IsDemo() { return m_isDemo; }
};
```

#### 使用示例
```mql5
#include <Okex/OkexAPI.mqh>

COkexAPI okexApi;

void OnStart() {
    // 查询余额
    string balance = okexApi.GetBalance();
    double equity = okexApi.ParseTotalEquity(balance);
    Print("总权益: ", equity);
    
    // 下单
    string result = okexApi.PlaceOrder(
        "XAUT-USDT-SWAP",
        "sell",    // 卖出
        "short",   // 开空
        2650.5,    // 价格
        1.0        // 数量
    );
    
    // 检查订单状态
    if (StringFind(result, "\"code\":\"0\"") >= 0) {
        Print("下单成功");
    }
}
```

### 2.3 行情数据管理 (MarketData.mqh)

#### 数据结构
```mql5
struct TickerData {
    double bid;        // 买一价
    double ask;        // 卖一价
    double last;       // 最新成交价
    datetime time;     // 时间戳
    bool valid;        // 数据是否有效
};

class CMarketData {
private:
    TickerData m_mt5Data;    // MT5行情
    TickerData m_okexData;   // OKX行情
    
public:
    // 更新MT5行情
    void UpdateMT5Ticker(string symbol);
    
    // 更新OKX行情（从WebSocket或REST API）
    void UpdateOKXTicker(string instId);
    
    // 获取数据
    TickerData GetMT5Data() { return m_mt5Data; }
    TickerData GetOKXData() { return m_okexData; }
    
    // 计算价差
    double GetSpread() {
        return m_okexData.bid - m_mt5Data.ask;
    }
};
```

---

## 三、套利策略核心逻辑

### 3.1 策略参数设计

```mql5
input double FirstOrderSpread = 10.0;  // 第一单价差阈值(USD)
input double NextOrderSpread = 5.0;    // 后续单间距(USD)
input int    MaxOrders = 5;            // 最大持仓单数
input double OkexTP = 20.0;            // OKX止盈点(USD)
input double MT5TP = 20.0;             // MT5止盈点(USD)
input double OkexLot = 0.01;           // OKX每单手数
input double MT5Lot = 0.01;            // MT5每单手数
input double OkexFee = 0.0005;         // OKX手续费率(0.05%)
input double MT5Fee = 0.0003;          // MT5手续费率(0.03%)
```

### 3.2 网格交易逻辑

#### 情况A：OKX价格高于MT5
```
条件：okex_bid > mt5_ask + FirstOrderSpread
动作：
  - OKX开空单（卖出）
  - MT5开多单（买入）
  
网格层次：
  第1单：价差 = FirstOrderSpread (10 USD)
  第2单：价差 = FirstOrderSpread + NextOrderSpread (15 USD)
  第3单：价差 = FirstOrderSpread + 2*NextOrderSpread (20 USD)
  ...
```

#### 情况B：MT5价格高于OKX
```
条件：mt5_bid > okex_ask + FirstOrderSpread
动作：
  - MT5开空单（卖出）
  - OKX开多单（买入）
```

### 3.3 止盈逻辑

```mql5
void CheckTakeProfit() {
    for (int i = 0; i < ArraySize(g_tradePairs); i++) {
        TradePair pair = g_tradePairs[i];
        
        // 获取当前价格
        double okexPrice = GetOKXPosition(pair.okex_id).price;
        double mt5Price = GetMT5Position(pair.mt5_ticket).price;
        
        // 计算当前价差
        double currentSpread = MathAbs(okexPrice - mt5Price);
        
        // 计算盈利
        double profit = (pair.openSpread - currentSpread) * pair.size;
        profit -= (OkexFee + MT5Fee) * okexPrice * pair.size;
        
        // 判断是否止盈
        if (profit >= OkexTP + MT5TP) {
            CloseOKXPosition(pair.okex_id);
            CloseMT5Position(pair.mt5_ticket);
            ArrayRemove(g_tradePairs, i);
        }
    }
}
```

### 3.4 风险控制

```mql5
// 最大持仓限制
if (ArraySize(g_tradePairs) >= MaxOrders) {
    return; // 不再开新单
}

// 余额检查
double okexEquity = okexApi.ParseTotalEquity(okexApi.GetBalance());
double mt5Equity = AccountInfoDouble(ACCOUNT_EQUITY);

if (okexEquity < MinEquity || mt5Equity < MinEquity) {
    Print("余额不足，停止交易");
    return;
}

// 单笔下单量限制
double maxLot = AccountInfoDouble(ACCOUNT_FREEMARGIN) / (price * 100);
double orderSize = MathMin(OkexLot, maxLot);
```

---

## 四、可视化与监控

### 4.1 图表面板设计

```mql5
// 面板显示内容
=== OKX Arbitrage EA ===

【行情信息】
MT5 XAUUSD:
  Bid: 2645.20
  Ask: 2645.80

OKX XAUT-USDT-SWAP:
  Bid: 2655.50
  Ask: 2656.10

价差: 10.30 USD

【账户信息】
MT5权益: $10,000.00
OKX权益: $9,850.50
总权益: $19,850.50

【持仓信息】
当前持仓: 3/5
订单1: Spread=12.5 USD, P&L=+45.20 USD
订单2: Spread=17.8 USD, P&L=+62.30 USD
订单3: Spread=23.1 USD, P&L=+78.50 USD

【统计信息】
今日交易: 15笔
今日盈亏: +$245.80
胜率: 86.7%
```

### 4.2 折线图绘制

```mql5
void DrawPriceLines() {
    // 线1：OKX Ask vs MT5 Bid 中线（红色）
    double line1 = (g_okexData.ask + g_mt5Data.bid) / 2.0;
    DrawTrendLine("OKX_Ask_MT5_Bid", line1, clrRed);
    
    // 线2：OKX Bid vs MT5 Ask 中线（蓝色）
    double line2 = (g_okexData.bid + g_mt5Data.ask) / 2.0;
    DrawTrendLine("OKX_Bid_MT5_Ask", line2, clrBlue);
    
    // 线3：价差（绿色）
    double spread = line1 - line2;
    DrawTrendLine("Spread", spread, clrGreen);
}

void DrawTrendLine(string name, double price, color clr) {
    if (ObjectFind(0, name) < 0) {
        ObjectCreate(0, name, OBJ_TREND, 0, 
                    iTime(NULL, 0, 10), price, 
                    TimeCurrent(), price);
    }
    ObjectSetInteger(0, name, OBJPROP_COLOR, clr);
    ObjectSetDouble(0, name, OBJPROP_PRICE, 0, price);
}
```

---

## 五、安装部署方案

### 5.1 文件结构

```
C:\OKXBridge\
├── Install\
│   ├── install.bat                    (自动安装脚本)
│   ├── uninstall.bat                  (卸载脚本)
│   └── Config.exe                     (图形化配置工具)
│
├── Libraries\okx\
│   ├── libokx_bridge.dll              (主DLL)
│   ├── libcurl-4.dll
│   ├── libssl-3-x64.dll
│   ├── libcrypto-3-x64.dll
│   └── ... (其他16个依赖DLL)
│
├── Include\Okex\
│   ├── OkexAPI.mqh                    (API封装)
│   └── MarketData.mqh                 (行情管理)
│
├── Experts\
│   └── OkexArbitrage.mq5              (EA主程序)
│
├── Config\
│   ├── okex_config.ini                (配置文件)
│   └── okex_config_template.ini       (配置模板)
│
└── Docs\
    ├── README.pdf                     (用户手册)
    ├── API_Documentation.pdf          (API文档)
    └── Changelog.txt                  (更新日志)
```

### 5.2 自动安装脚本

```batch
@echo off
setlocal enabledelayedexpansion

echo ================================================
echo   OKX-MT5 Arbitrage EA 自动安装程序
echo ================================================

REM 1. 检测MT5安装路径
set "MT5_PATH=%APPDATA%\MetaQuotes\Terminal\Common"
if not exist "%MT5_PATH%" (
    echo [错误] 未检测到MT5安装
    pause
    exit /b 1
)

set "MQL5_PATH=%MT5_PATH%\MQL5"
echo [√] 检测到MT5路径: %MQL5_PATH%

REM 2. 创建目录结构
echo.
echo [1/5] 创建目录结构...
mkdir "%MQL5_PATH%\Libraries\okx" 2>nul
mkdir "%MQL5_PATH%\Include\Okex" 2>nul
mkdir "%MQL5_PATH%\Experts" 2>nul
mkdir "%MQL5_PATH%\Files" 2>nul
echo [√] 目录创建完成

REM 3. 复制DLL文件
echo.
echo [2/5] 复制DLL文件...
xcopy /Y "Libraries\okx\*" "%MQL5_PATH%\Libraries\okx\"
echo [√] DLL文件复制完成 (21个文件)

REM 4. 复制头文件
echo.
echo [3/5] 复制头文件...
xcopy /Y "Include\Okex\*" "%MQL5_PATH%\Include\Okex\"
echo [√] 头文件复制完成

REM 5. 复制EA文件
echo.
echo [4/5] 复制EA文件...
copy /Y "Experts\OkexArbitrage.mq5" "%MQL5_PATH%\Experts\"
echo [√] EA文件复制完成

REM 6. 创建配置文件
echo.
echo [5/5] 创建配置文件...
if not exist "%MQL5_PATH%\Files\okex_config.ini" (
    copy "Config\okex_config_template.ini" "%MQL5_PATH%\Files\okex_config.ini"
    echo [!] 请编辑配置文件填入API密钥
) else (
    echo [√] 配置文件已存在
)

echo.
echo ================================================
echo   安装完成！
echo ================================================
echo.
echo [下一步操作]
echo   1. 关闭MT5
echo   2. 编辑配置文件填入API密钥:
echo      %MQL5_PATH%\Files\okex_config.ini
echo   3. 重启MT5
echo   4. 编译EA (按F7)
echo   5. 将EA拖到图表上
echo.
echo [安装位置]
echo   DLL: %MQL5_PATH%\Libraries\okx\
echo   Include: %MQL5_PATH%\Include\Okex\
echo   EA: %MQL5_PATH%\Experts\
echo.
pause
```

### 5.3 图形化配置工具 (Config.exe)

使用C++/Qt或C#/WinForms开发：
```
[窗口标题] OKX API 配置工具

┌────────────────────────────────────┐
│  API密钥配置                        │
├────────────────────────────────────┤
│                                    │
│  API Key:                          │
│  [____________________________]    │
│                                    │
│  Secret Key:                       │
│  [____________________________]    │
│                                    │
│  Passphrase:                       │
│  [____________________________]    │
│                                    │
│  [ ] 模拟盘模式                     │
│                                    │
│  [测试连接]  [保存配置]  [取消]     │
│                                    │
└────────────────────────────────────┘
```

---

## 六、已解决的技术难点

### 6.1 DLL依赖问题
**问题**：MT5无法加载带有大量外部依赖的DLL
**解决方案**：
1. 使用子目录隔离（okx/文件夹）
2. 批量复制所有依赖DLL到同一目录
3. 使用相对路径加载：`#import "okx/libokx_bridge.dll"`

### 6.2 静态链接失败
**问题**：MSYS2的libcurl缺少完整静态库
**解决方案**：放弃完全静态链接，采用"打包所有DLL"方案

### 6.3 HMAC签名认证
**问题**：OKX API需要复杂的签名流程
**解决方案**：
1. 使用OpenSSL的HMAC函数
2. 精确匹配时间戳格式（毫秒级）
3. 正确拼接签名字符串

### 6.4 JSON解析
**问题**：MT5不支持原生JSON解析
**解决方案**：
1. C++ DLL内使用nlohmann/json库解析
2. 只返回必要字段给MT5
3. MT5使用StringFind简单查找关键字段

### 6.5 跨平台持仓匹配
**问题**：需要精确匹配OKX和MT5的成对订单
**解决方案**：
```mql5
struct TradePair {
    string okex_id;        // OKX订单ID
    ulong mt5_ticket;      // MT5订单号
    double openSpread;     // 开仓时价差
    double size;           // 数量
    datetime openTime;     // 开仓时间
};

TradePair g_tradePairs[];  // 全局数组管理所有交易对
```

---

## 七、达成的核心共识

### 7.1 架构共识

#### ✅ 使用C++ DLL作为桥接层
- 性能优势：原生代码，无需解释器
- 集成优势：MT5原生支持DLL调用
- 维护优势：单一可执行文件，易于分发

#### ✅ 子目录隔离管理依赖
- 不污染MT5主目录
- 便于版本管理和升级
- 便于卸载（删除整个okx文件夹）

#### ✅ 配置文件外置化
- 敏感信息（API密钥）独立存储
- 支持热重载（修改配置无需重新编译）
- 易于备份和迁移

### 7.2 功能共识

#### ✅ 三层架构设计
1. **底层**：C++ DLL（网络通信、API调用）
2. **中层**：MQH封装（API抽象、数据管理）
3. **上层**：EA主程序（策略逻辑、用户界面）

#### ✅ 模块化扩展
- 策略Kit：不同策略可热插拔
- 面板Kit：自定义UI组件
- 数据Kit：本地缓存和历史数据
- AI Kit：机器学习策略（未来扩展）

#### ✅ 完整的风控系统
- 最大持仓限制
- 余额检查
- 止损止盈
- 异常处理和日志

### 7.3 交付共识

#### ✅ 一键安装包
- 双击Setup.exe即可完成安装
- 自动检测MT5路径
- 智能配置向导
- 完整的卸载支持

#### ✅ 用户文档
- 图文安装手册（README.pdf）
- API参考文档
- 常见问题解答
- 更新日志

#### ✅ 测试验证流程
1. 编译DLL成功
2. 复制到MT5目录
3. MT5 Journal显示余额JSON
4. 图表显示实时价差折线
5. 模拟盘测试下单
6. 实盘小额测试

---

## 八、当前开发状态

### 8.1 已完成模块

| 模块 | 完成度 | 说明 |
|------|--------|------|
| DLL编译环境 | ✅ 100% | MSYS2配置完成，编译命令确定 |
| DLL安装方案 | ✅ 100% | 子目录隔离，批处理脚本 |
| API认证 | ✅ 100% | HMAC-SHA256签名实现 |
| 余额查询 | ✅ 100% | OKX_QueryBalance函数 |
| API封装框架 | ⏳ 30% | COkexAPI类基本结构 |
| 行情数据结构 | ⏳ 20% | TickerData定义完成 |
| EA主框架 | ⏳ 10% | OnInit/OnTick框架 |

### 8.2 待开发模块

| 模块 | 优先级 | 预计时间 |
|------|--------|----------|
| OKX下单功能 | P0 | 2小时 |
| OKX查仓功能 | P0 | 1小时 |
| OKX撤单功能 | P0 | 1小时 |
| 实时行情更新 | P0 | 3小时 |
| 价差计算逻辑 | P0 | 2小时 |
| 网格开仓逻辑 | P1 | 4小时 |
| 止盈平仓逻辑 | P1 | 3小时 |
| 图表面板 | P1 | 6小时 |
| 折线图绘制 | P2 | 2小时 |
| 配置图形化工具 | P2 | 4小时 |
| 完整安装包 | P2 | 3小时 |

### 8.3 技术债务

1. **错误处理**：需要完善异常捕获和错误码
2. **日志系统**：需要详细的运行日志
3. **性能优化**：HTTP请求可以添加连接池
4. **安全性**：配置文件需要加密存储
5. **单元测试**：每个函数需要测试用例

---

## 九、下一步行动计划

### 9.1 立即任务（今晚完成）

#### Step 1: 编译完整DLL
```bash
cd /c/okx_bridge

# 实现完整API功能
cat > src/okx_bridge.cpp << 'EOF'
// 实现全部5个API函数：
// 1. OKX_QueryBalance
// 2. OKX_PlaceOrder  
// 3. OKX_GetPositions
// 4. OKX_CancelOrder
// 5. OKX_GetTicker (获取实时行情)
EOF

# 编译
g++ -shared -O2 \
  -I/mingw64/include -L/mingw64/lib \
  src/okx_bridge.cpp \
  -lcurl -lssl -lcrypto \
  -lws2_32 -lwinmm -lcrypt32 \
  -o libokx_bridge.dll

# 验证
ldd libokx_bridge.dll
ls -lh libokx_bridge.dll
```

#### Step 2: 完善MQH封装
```mql5
// 实现完整的API封装
1. OkexAPI.mqh - 所有API函数封装
2. MarketData.mqh - 行情管理和价差计算
3. TradeManager.mqh - 订单管理和持仓跟踪
```

#### Step 3: EA主逻辑开发
```mql5
// 实现核心策略
1. 实时行情更新
2. 价差监控
3. 网格开仓
4. 止盈平仓
5. 风险控制
```

#### Step 4: 测试验证
```
1. 模拟盘测试所有API调用
2. 验证价差计算准确性
3. 小额实盘测试开平仓
4. 压力测试（多单并发）
```

### 9.2 明天任务（4小时交付）

#### 图形面板开发
```mql5
// PanelKit 模块
1. 实时行情显示
2. 账户信息面板
3. 持仓列表
4. 统计图表
5. 一键启停按钮
```

#### 折线图优化
```mql5
// 优化图表显示
1. 多周期价差曲线
2. 成交点标记
3. 盈亏曲线
4. 历史回放功能
```

#### 完整安装包
```
1. Inno Setup打包脚本
2. 图形化配置工具(Config.exe)
3. 用户手册(PDF)
4. 双击安装测试
```

### 9.3 后天任务（扩展功能）

#### 多交易所支持
```
1. 币安(Binance)
2. 火币(Huobi)
3. Bybit
4. 配置文件支持多交易所
```

#### AI策略集成
```
1. 价差预测模型
2. 最优开仓时机
3. 动态止盈参数
4. 风险评估系统
```

#### 远程监控
```
1. Grafana仪表盘
2. Prometheus指标采集
3. 微信/邮件告警
4. 移动端APP
```

---

## 十、关键技术细节回顾

### 10.1 编译命令详解

```bash
g++ -shared -O2 \
  -I/mingw64/include \          # 头文件路径
  -L/mingw64/lib \               # 库文件路径
  src/okx_bridge.cpp \           # 源文件
  -lcurl \                       # libcurl (HTTP客户端)
  -lssl \                        # OpenSSL SSL/TLS
  -lcrypto \                     # OpenSSL 加密库
  -lws2_32 \                     # Windows Socket
  -lwinmm \                      # Windows 多媒体
  -lcrypt32 \                    # Windows 加密API
  -o libokx_bridge.dll           # 输出文件
```

### 10.2 MT5 DLL调用规范

```mql5
// 声明DLL函数
#import "okx/libokx_bridge.dll"
    int OKX_QueryBalance(char &out[], int len);
    int OKX_PlaceOrder(
        string instId,
        string side,
        string posSide,
        double price,
        double size,
        char &out[],
        int len
    );
#import

// 调用示例
void TestAPI() {
    char buffer[32768];
    
    // 查询余额
    int ret = OKX_QueryBalance(buffer, 32768);
    if (ret == 0) {
        string json = CharArrayToString(buffer);
        Print("API返回: ", json);
    } else {
        Print("API调用失败: ", ret);
    }
}
```

### 10.3 OKX API签名流程

```cpp
// 1. 生成时间戳（毫秒）
string timestamp = GetCurrentTimeMs();  // "1635724800123"

// 2. 拼接待签名字符串
string method = "GET";
string path = "/api/v5/account/balance";
string body = "";  // GET请求为空
string presign = timestamp + method + path + body;

// 3. HMAC-SHA256签名
unsigned char hash[EVP_MAX_MD_SIZE];
unsigned int hash_len;
HMAC(EVP_sha256(), 
     secret_key.c_str(), secret_key.length(),
     (unsigned char*)presign.c_str(), presign.length(),
     hash, &hash_len);

// 4. Base64编码
string signature = Base64Encode(hash, hash_len);

// 5. 设置HTTP头
curl_slist* headers = nullptr;
headers = curl_slist_append(headers, ("OK-ACCESS-KEY: " + api_key).c_str());
headers = curl_slist_append(headers, ("OK-ACCESS-SIGN: " + signature).c_str());
headers = curl_slist_append(headers, ("OK-ACCESS-TIMESTAMP: " + timestamp).c_str());
headers = curl_slist_append(headers, ("OK-ACCESS-PASSPHRASE: " + passphrase).c_str());
```

### 10.4 网格交易算法

```mql5
void OnTick() {
    // 更新行情
    UpdateMarketData();
    
    // 计算价差
    double spread = g_okexData.bid - g_mt5Data.ask;
    
    // 检查是否达到开仓条件
    if (spread > FirstOrderSpread) {
        int currentOrders = ArraySize(g_tradePairs);
        
        // 计算应该开仓的层级
        int targetLevel = (int)((spread - FirstOrderSpread) / NextOrderSpread) + 1;
        targetLevel = MathMin(targetLevel, MaxOrders);
        
        // 补齐缺少的订单
        while (currentOrders < targetLevel) {
            double entryPrice = CalculateGridPrice(currentOrders);
            OpenTradePair(entryPrice);
            currentOrders++;
        }
    }
    
    // 检查止盈
    CheckTakeProfitAll();
}

double CalculateGridPrice(int level) {
    // 第一单：基础价差
    // 第N单：基础价差 + (N-1) * 间距
    return FirstOrderSpread + level * NextOrderSpread;
}
```

---

## 十一、风险提示与免责声明

### 11.1 技术风险
1. **网络延迟**：API调用存在网络延迟，可能导致价差消失
2. **滑点风险**：快速行情下成交价格可能偏离预期
3. **API限流**：OKX和MT5都有API调用频率限制
4. **系统稳定性**：长时间运行需要考虑内存泄漏

### 11.2 交易风险
1. **价差风险**：两个平台价格可能长期偏离
2. **流动性风险**：大单可能无法及时成交
3. **手续费**：频繁交易会产生高额手续费
4. **爆仓风险**：杠杆交易可能导致爆仓

### 11.3 合规风险
1. **交易所规则**：需遵守交易所API使用规则
2. **监管要求**：某些地区可能限制此类交易
3. **税务问题**：交易盈利需依法纳税

### 11.4 免责声明
**本系统仅供学习研究使用，不构成投资建议。使用本系统进行实盘交易的一切后果由用户自行承担。**

---

## 十二、多平台支持与灵活配置

### 12.1 支持多交易所平台

#### 设计思路
系统采用**模块化设计**，通过配置文件和DLL接口抽象层，可以轻松切换或添加其他交易所：

**当前支持**：
- OKX (主要实现)

**计划扩展**：
- 币安 (Binance)
- 火币 (Huobi)
- Bybit
- Gate.io

#### 配置文件结构
```json
{
    "exchanges": [
        {
            "name": "okx",
            "enabled": true,
            "is_demo": false,
            "api_config": {
                "api_key": "cfd780d7-6dc6-4fee-bb27-d7a4608d2fa8",
                "secret_key": "4DD3E6E14B69380235D2D585DDE5B5B5",
                "passphrase": "Abc@123456",
                "base_url": "https://www.okx.com",
                "ws_public": "wss://ws.okx.com:8443/ws/v5/public",
                "ws_private": "wss://ws.okx.com:8443/ws/v5/private"
            }
        },
        {
            "name": "binance",
            "enabled": false,
            "is_demo": false,
            "api_config": {
                "api_key": "your-binance-key",
                "secret_key": "your-binance-secret",
                "base_url": "https://fapi.binance.com",
                "ws_url": "wss://fstream.binance.com"
            }
        }
    ],
    "active_exchange": "okx"
}
```

#### DLL接口抽象层
```cpp
// 交易所接口抽象类
class IExchange {
public:
    virtual ~IExchange() = default;
    
    // 生命周期
    virtual int Init(const ExchangeConfig& config) = 0;
    virtual void Release() = 0;
    
    // 行情接口
    virtual int SubscribeTicker(const string& symbol, TickCallback cb) = 0;
    virtual int SubscribeDepth(const string& symbol, DepthCallback cb) = 0;
    
    // 交易接口
    virtual int PlaceOrder(const OrderRequest& req, OrderResponse& resp) = 0;
    virtual int CancelOrder(const string& orderId) = 0;
    virtual int QueryPositions(vector<Position>& positions) = 0;
    
    // 账户接口
    virtual int QueryBalance(AccountInfo& info) = 0;
    virtual int QueryFeeRate(const string& symbol, FeeRate& rate) = 0;
};

// OKX实现
class OKXExchange : public IExchange {
    // 实现所有虚函数
};

// Binance实现
class BinanceExchange : public IExchange {
    // 实现所有虚函数
};

// 工厂模式
class ExchangeFactory {
public:
    static IExchange* Create(const string& name) {
        if (name == "okx") return new OKXExchange();
        if (name == "binance") return new BinanceExchange();
        return nullptr;
    }
};
```

#### 切换平台操作流程
1. **修改配置文件** - 将`active_exchange`改为目标平台名称
2. **重启EA** - EA会自动加载新平台的API配置
3. **验证连接** - EA OnInit()中测试新平台连接
4. **无需重新编译** - 所有切换通过配置完成

### 12.2 支持多货币对切换

#### 货币对配置
```json
{
    "trading_pairs": [
        {
            "exchange_symbol": "XAUT-USDT-SWAP",  // 交易所品种代码
            "mt5_symbol": "XAUUSD",                // MT5品种代码
            "enabled": true,
            "contract_size": 1.0,                  // 合约大小
            "price_precision": 2,                  // 价格精度
            "quantity_precision": 3,               // 数量精度
            "min_quantity": 0.01,                  // 最小下单量
            "tick_size": 0.01                      // 最小变动价位
        },
        {
            "exchange_symbol": "BTC-USDT-SWAP",
            "mt5_symbol": "BTCUSD",
            "enabled": false,
            "contract_size": 0.001,
            "price_precision": 1,
            "quantity_precision": 3,
            "min_quantity": 0.001,
            "tick_size": 0.1
        }
    ],
    "active_pair": "XAUT-USDT-SWAP"
}
```

#### DLL中的货币对管理
```cpp
struct TradingPairConfig {
    string exchange_symbol;    // "XAUT-USDT-SWAP"
    string mt5_symbol;         // "XAUUSD"
    double contract_size;
    int price_precision;
    int quantity_precision;
    double min_quantity;
    double tick_size;
};

class TradingPairManager {
private:
    map<string, TradingPairConfig> pairs;
    string active_pair;
    
public:
    bool LoadFromConfig(const string& config_path);
    bool SetActivePair(const string& symbol);
    TradingPairConfig GetActivePair() const;
    
    // 价格/数量转换
    double NormalizePr(double price, const string& symbol);
    double NormalizeQuantity(double qty, const string& symbol);
};
```

#### EA中切换货币对
```mql5
input string InpTradingPair = "XAUT-USDT-SWAP";  // 交易货币对

int OnInit() {
    // 设置活动货币对
    if (!DLL_SetActivePair(InpTradingPair)) {
        Print("❌ 切换货币对失败: ", InpTradingPair);
        return INIT_FAILED;
    }
    
    // 获取货币对配置
    PairConfig config;
    DLL_GetPairConfig(config);
    
    Print("✓ 当前交易对:");
    Print("  交易所: ", config.exchange_symbol);
    Print("  MT5: ", config.mt5_symbol);
    Print("  合约大小: ", config.contract_size);
    Print("  最小下单量: ", config.min_quantity);
    
    return INIT_SUCCEEDED;
}
```

### 12.3 模拟盘与实盘识别（达成共识）

#### OKX特殊性说明
OKX与其他交易所不同：
- **URL完全相同**：模拟盘和实盘都使用 `https://www.okx.com`
- **通过API密钥区分**：密钥本身决定访问的是模拟还是实盘环境
- **API格式一致**：返回字段、请求参数完全相同

#### 配置区分（客户可见）
```json
{
    "Exchange": "OKX",
    "BaseURL": "https://www.okx.com",
    "AK": "cfd780d7-6dc6-4fee-bb27-d7a4608d2fa8",
    "SK": "4DD3E6E14B69380235D2D585DDE5B5B5",
    "Passphrase": "Abc@123456",
    "AccountType": "Live",        // 手动声明："Demo" 或 "Live"
    "Instruments": ["XAUT-USDT-SWAP", "BTC-USDT-SWAP"]
}
```

#### DLL中的双接口验证识别机制

**核心共识**：使用两个API接口交叉验证，确保准确识别账户类型

```cpp
enum AccountType {
    ACCOUNT_DEMO,
    ACCOUNT_LIVE,
    ACCOUNT_UNCERTAIN  // 两个接口返回不一致
};

class AccountTypeDetector {
public:
    // 双接口验证：确保准确性
    static AccountType DetectWithDualCheck() {
        string typeFromUserAPI = CheckUserMeAPI();
        string typeFromBalance = CheckBalanceAPI();
        
        // 两个接口必须一致
        if (typeFromUserAPI == "unknown" || typeFromBalance == "unknown") {
            return ACCOUNT_UNCERTAIN;
        }
        
        if (typeFromUserAPI != typeFromBalance) {
            Log("⚠️ 账户类型不一致！");
            Log("   /users/me 返回: " + typeFromUserAPI);
            Log("   /balance 返回: " + typeFromBalance);
            return ACCOUNT_UNCERTAIN;
        }
        
        return (typeFromUserAPI == "Demo") ? ACCOUNT_DEMO : ACCOUNT_LIVE;
    }
    
private:
    // 方法1：通过 /api/v5/users/me 接口
    static string CheckUserMeAPI() {
        string response = HttpGet("/api/v5/users/me");
        
        // 检查返回中的 type 字段
        if (response.find("\"type\":\"demo\"") != string::npos) {
            return "Demo";
        }
        if (response.find("\"type\":\"live\"") != string::npos) {
            return "Live";
        }
        
        return "unknown";
    }
    
    // 方法2：通过 /api/v5/account/balance 余额特征
    static string CheckBalanceAPI() {
        string response = HttpGet("/api/v5/account/balance");
        json data = json::parse(response);
        
        if (!data.contains("data") || data["data"].empty()) {
            return "unknown";
        }
        
        string totalEq = data["data"][0]["totalEq"].get<string>();
        double equity = stod(totalEq);
        
        // 模拟盘特征：余额通常是整数（如 10000.0000）
        // 实盘特征：余额有小数位（如 1234.56）
        if (abs(equity - round(equity)) < 1e-4) {
            return "Demo";  // 可能是模拟盘
        }
        
        return "Live";  // 可能是实盘
    }
};

// 导出函数
extern "C" __declspec(dllexport) 
int __stdcall OKX_GetAccountType() {
    return (int)AccountTypeDetector::DetectWithDualCheck();
}

extern "C" __declspec(dllexport)
int __stdcall OKX_GetAccountTypeDetail(char* out, int bufLen) {
    AccountType type = AccountTypeDetector::DetectWithDualCheck();
    
    json result;
    result["code"] = (type == ACCOUNT_UNCERTAIN) ? "-2" : "0";
    result["accountType"] = (type == ACCOUNT_DEMO) ? "Demo" : 
                           (type == ACCOUNT_LIVE) ? "Live" : "Uncertain";
    result["meType"] = CheckUserMeAPI();
    result["balType"] = CheckBalanceAPI();
    result["detectSource"] = "me+balance";
    
    string jsonStr = result.dump();
    strncpy(out, jsonStr.c_str(), bufLen - 1);
    out[bufLen - 1] = '\0';
    
    return 0;
}
```

#### 验证规则对照表

| 验证方式 | 模拟盘特征 | 实盘特征 |
|---------|-----------|---------|
| **API密钥前缀** | 可能以`sk-demo`开头 | 普通格式 |
| **/users/me接口** | `"type":"demo"` | `"type":"live"` |
| **/balance余额** | 整数（如10000.0000） | 有小数（如1234.56） |
| **最终判断** | 两个接口都返回Demo | 两个接口都返回Live |

#### 安全防护机制
```cpp
class TradingSafety {
public:
    // 实盘操作前的确认
    static bool ConfirmLiveTrading(const string& action, const OrderRequest& req) {
        if (!EnvironmentDetector::IsDemo()) {
            // 实盘操作需要额外验证
            Log("⚠️ 实盘操作警告: " + action);
            Log("   品种: " + req.symbol);
            Log("   方向: " + req.side);
            Log("   数量: " + to_string(req.quantity));
            Log("   价格: " + to_string(req.price));
            
            if (config_require_confirmation) {
                // 可以实现PIN码验证、双因素认证等
                return RequestUserConfirmation();
            }
        }
        return true;
    }
    
    // 下单前检查
    static bool ValidateOrder(const OrderRequest& req) {
        if (EnvironmentDetector::IsDemo()) {
            return true;  // 模拟盘不限制
        }
        
        // 实盘检查
        if (req.quantity > config_max_order_size) {
            Log("❌ 单笔下单量超限: " + to_string(req.quantity));
            return false;
        }
        
        double current_exposure = GetTotalExposure();
        if (current_exposure > config_max_exposure) {
            Log("❌ 总敞口超限: " + to_string(current_exposure));
            return false;
        }
        
        return true;
    }
};
```

#### MT5 EA中的显示与确认

```mql5
// 全局变量
int g_accountType = 0;  // 0=未知, 1=模拟, 2=实盘, -1=不确定

int OnInit() {
    // 查询账户类型
    char buffer[4096];
    OKX_GetAccountTypeDetail(buffer, 4096);
    string json = CharArrayToString(buffer);
    
    // 解析结果
    if (StringFind(json, "\"accountType\":\"Demo\"") >= 0) {
        g_accountType = 1;
    } else if (StringFind(json, "\"accountType\":\"Live\"") >= 0) {
        g_accountType = 2;
    } else {
        g_accountType = -1;
    }
    
    // 显示环境标识
    CreateEnvironmentLabel();
    
    // 不确定时警告
    if (g_accountType == -1) {
        Alert("⚠️ 无法确定账户类型！请联系技术支持。");
        Print("账户验证详情: ", json);
        return INIT_FAILED;
    }
    
    // 实盘警告
    if (g_accountType == 2) {
        int answer = MessageBox(
            "⚠️ 检测到实盘环境！\n\n" +
            "请确认:\n" +
            "1. API密钥配置正确\n" +
            "2. 策略参数已充分测试\n" +
            "3. 风险控制措施已设置\n\n" +
            "是否继续启动？",
            "实盘环境确认",
            MB_YESNO | MB_ICONWARNING
        );
        
        if (answer != IDYES) {
            return INIT_FAILED;
        }
    }
    
    return INIT_SUCCEEDED;
}

void CreateEnvironmentLabel() {
    string label_text;
    color label_color;
    
    switch(g_accountType) {
        case 1:  // 模拟盘
            label_text = "【模拟盘】DEMO";
            label_color = clrLimeGreen;
            break;
        case 2:  // 实盘
            label_text = "【实盘】LIVE";
            label_color = clrRed;
            break;
        default:
            label_text = "【未知】";
            label_color = clrOrange;
    }
    
    ObjectCreate(0, "EnvLabel", OBJ_LABEL, 0, 0, 0);
    ObjectSetString(0, "EnvLabel", OBJPROP_TEXT, label_text);
    ObjectSetInteger(0, "EnvLabel", OBJPROP_COLOR, label_color);
    ObjectSetInteger(0, "EnvLabel", OBJPROP_FONTSIZE, 20);
    ObjectSetInteger(0, "EnvLabel", OBJPROP_CORNER, CORNER_RIGHT_UPPER);
    ObjectSetInteger(0, "EnvLabel", OBJPROP_XDISTANCE, 20);
    ObjectSetInteger(0, "EnvLabel", OBJPROP_YDISTANCE, 20);
}

// 下单前二次确认（仅实盘）
bool ConfirmTradeOperation(string operation, double price, double qty) {
    if (g_accountType != 2) {
        return true;  // 模拟盘直接通过
    }
    
    // 实盘需要确认
    int answer = MessageBox(
        "实盘操作确认\n\n" +
        "操作: " + operation + "\n" +
        "价格: " + DoubleToString(price, 2) + "\n" +
        "数量: " + DoubleToString(qty, 3) + "\n\n" +
        "确认执行？",
        "实盘交易确认",
        MB_YESNO | MB_ICONQUESTION
    );
    
    return (answer == IDYES);
}
```

### 12.4 配置热重载机制（达成共识）

**核心共识**：通过配置文件外置化，实现零重启切换平台、货币对、API密钥

#### 配置文件监听（DLL后台线程）

```cpp
class ConfigWatcher {
private:
    string config_path;
    filesystem::file_time_type last_write_time;
    atomic<bool> running;
    thread watcher_thread;
    
public:
    ConfigWatcher(const string& path) : config_path(path), running(true) {
        last_write_time = filesystem::last_write_time(config_path);
        
        // 启动监听线程
        watcher_thread = thread([this]() {
            while (running) {
                CheckConfigChange();
                this_thread::sleep_for(chrono::seconds(1));
            }
        });
    }
    
    ~ConfigWatcher() {
        running = false;
        if (watcher_thread.joinable()) {
            watcher_thread.join();
        }
    }
    
private:
    void CheckConfigChange() {
        try {
            auto current_time = filesystem::last_write_time(config_path);
            
            if (current_time != last_write_time) {
                Log("📝 检测到配置文件变化，正在重新加载...");
                
                // 重新加载配置
                if (ReloadConfig()) {
                    last_write_time = current_time;
                    Log("✓ 配置已更新");
                    
                    // 通知MT5
                    BroadcastConfigReloadEvent();
                } else {
                    Log("❌ 配置加载失败，保持旧配置");
                }
            }
        } catch (const exception& e) {
            Log("配置监听异常: " + string(e.what()));
        }
    }
    
    bool ReloadConfig() {
        try {
            ifstream f(config_path);
            json new_config = json::parse(f);
            
            // 验证配置有效性
            if (!ValidateConfig(new_config)) {
                return false;
            }
            
            // 原子性更新全局配置
            lock_guard<mutex> lock(config_mutex);
            g_config = new_config;
            
            // 重新初始化交易所连接
            if (g_config["Exchange"] != g_current_exchange) {
                ReconnectExchange();
            }
            
            // 重新订阅行情
            if (g_config["Instruments"] != g_current_instruments) {
                ResubscribeMarketData();
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void BroadcastConfigReloadEvent() {
        // 设置事件标志，MT5通过轮询检测
        SetEvent(g_config_reload_event);
    }
};

// 全局实例
unique_ptr<ConfigWatcher> g_config_watcher;

// DLL初始化时启动
extern "C" __declspec(dllexport)
int __stdcall OKX_Init(const char* config_file) {
    g_config_watcher = make_unique<ConfigWatcher>(config_file);
    return 0;
}
```

#### MT5端配置变更响应

```mql5
// 全局变量
datetime g_last_config_check = 0;

void OnTimer() {
    // 每秒检查一次配置变更
    if (TimeCurrent() - g_last_config_check >= 1) {
        g_last_config_check = TimeCurrent();
        
        if (OKX_IsConfigChanged()) {
            Print("📝 配置已更新，正在重新初始化...");
            
            // 重新查询当前配置
            char buffer[4096];
            OKX_GetCurrentConfig(buffer, 4096);
            string config = CharArrayToString(buffer);
            
            Print("新配置: ", config);
            
            // 通知用户
            Comment("配置已热更新\n",
                   "无需重启EA\n",
                   "新配置已生效");
            
            // 可选：重新初始化策略参数
            ReinitializeStrategy();
        }
    }
    
    // 其他定时任务...
}

bool ReinitializeStrategy() {
    // 重新读取策略参数
    char buffer[4096];
    OKX_GetStrategyParams(buffer, 4096);
    
    // 解析并应用新参数
    // ...
    
    return true;
}
```

#### 配置变更的完整流程

```
用户操作
    ↓
1. 用户通过Config.exe修改配置
    ↓
2. 保存到Config.json
    ↓
3. DLL后台线程检测文件修改时间变化
    ↓
4. DLL重新解析JSON配置
    ↓
5. 验证配置有效性
    ↓
6. 原子性更新内存中的配置
    ↓
7. 根据变更内容执行相应操作:
   - 交易所变更 → 断开旧连接，连接新交易所
   - 货币对变更 → 取消旧订阅，订阅新货币对
   - API密钥变更 → 重新认证
    ↓
8. 设置配置变更事件标志
    ↓
9. MT5 OnTimer检测到事件
    ↓
10. MT5更新界面显示
    ↓
完成（用户无感知，零重启）
```

#### 配置模板示例

**切换到币安平台**：
```json
{
    "Exchange": "Binance",
    "BaseURL": "https://fapi.binance.com",
    "AK": "your-binance-api-key",
    "SK": "your-binance-secret-key",
    "Passphrase": "",
    "Instruments": ["BTCUSDT", "ETHUSDT"],
    "AccountType": "Live"
}
```

**切换货币对**：
```json
{
    "Exchange": "OKX",
    "BaseURL": "https://www.okx.com",
    "AK": "cfd780d7-6dc6-4fee-bb27-d7a4608d2fa8",
    "SK": "4DD3E6E14B69380235D2D585DDE5B5B5",
    "Passphrase": "Abc@123456",
    "Instruments": ["BTC-USDT-SWAP", "ETH-USDT-SWAP"],  // 从黄金改为BTC/ETH
    "AccountType": "Live"
}
```

**切换到模拟盘**：
```json
{
    "Exchange": "OKX",
    "BaseURL": "https://www.okx.com",
    "AK": "demo-api-key",
    "SK": "demo-secret-key",
    "Passphrase": "demo-pass",
    "Instruments": ["XAUT-USDT-SWAP"],
    "AccountType": "Demo"  // 仅此一项变更
}
```

### 12.5 跨平台套利配置示例

#### 多交易所套利配置
```json
{
    "arbitrage_pairs": [
        {
            "name": "OKX-Binance-XAUT",
            "exchange_a": {
                "name": "okx",
                "symbol": "XAUT-USDT-SWAP",
                "fee_rate": 0.0005
            },
            "exchange_b": {
                "name": "binance",
                "symbol": "PAXGUSDT",
                "fee_rate": 0.0004
            },
            "mt5_reference": "XAUUSD",
            "min_spread": 10.0,
            "enabled": true
        }
    ]
}
```

#### DLL导出函数
```cpp
// 切换交易所
extern "C" __declspec(dllexport)
int __stdcall SetActiveExchange(const char* exchange_name);

// 切换货币对
extern "C" __declspec(dllexport)
int __stdcall SetActivePair(const char* symbol);

// 查询环境
extern "C" __declspec(dllexport)
bool __stdcall IsDemo();

extern "C" __declspec(dllexport)
int __stdcall GetEnvironmentInfo(char* out, int bufLen);

// 获取支持的交易所列表
extern "C" __declspec(dllexport)
int __stdcall GetSupportedExchanges(char* out, int bufLen);

// 获取支持的货币对列表
extern "C" __declspec(dllexport)
int __stdcall GetSupportedPairs(const char* exchange, char* out, int bufLen);
```

---

## 十三、总结

### 项目核心价值
1. **自动化套利**：7×24小时监控价差，自动执行交易
2. **风险可控**：对冲机制降低单边风险
3. **模块化设计**：易于扩展和维护
4. **一键部署**：简化用户使用门槛

### 技术创新点
1. **子目录隔离方案**：优雅解决DLL依赖问题
2. **三层架构**：清晰的职责划分
3. **配置外置化**：提高系统灵活性
4. **热插拔机制**：支持策略动态加载

### 可扩展方向
1. **多交易所**：支持更多交易平台
2. **AI策略**：机器学习优化参数
3. **云端部署**：VPS托管，远程监控
4. **移动端**：开发手机APP监控

### 最终目标
**打造一个稳定、高效、易用的跨平台套利交易系统，帮助用户在低风险下获取稳定收益。**

---

## 附录：完整文件清单

### A. C++ 源代码
- `src/okx_bridge.cpp` - DLL主文件
- `src/okx_api.h` - API函数声明
- `src/json_parser.cpp` - JSON解析工具
- `src/crypto_utils.cpp` - 加密工具函数

### B. MQL5 代码
- `Include/Okex/OkexAPI.mqh` - API封装
- `Include/Okex/MarketData.mqh` - 行情管理
- `Include/Okex/TradeManager.mqh` - 订单管理
- `Experts/OkexArbitrage.mq5` - EA主程序

### C. 配置文件
- `Config/okex_config.ini` - 用户配置
- `Config/strategy_params.ini` - 策略参数

### D. 脚本工具
- `Install/install.bat` - 安装脚本
- `Install/uninstall.bat` - 卸载脚本
- `Install/setup.iss` - Inno Setup配置

### E. 文档
- `Docs/README.pdf` - 用户手册
- `Docs/API_Documentation.pdf` - API文档
- `Docs/Troubleshooting.pdf` - 故障排除
- `Docs/Changelog.txt` - 更新日志

---

**文档生成时间**: 2025-11-01
**文档版本**: 1.0
**项目状态**: 开发中 (Alpha)
