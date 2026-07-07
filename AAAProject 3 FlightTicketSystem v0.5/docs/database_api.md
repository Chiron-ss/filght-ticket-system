# 数据库模块接口文档 (Database API)

## 1. 简介 (Introduction)
`DatabaseManager` 类是本系统的核心数据管理模块。它负责所有与数据库的交互，包括用户注册登录、航班查询、订单处理等。
为了方便大家使用，该类采用了 **单例模式 (Singleton Pattern)**。这意味着你不需要自己创建 `DatabaseManager` 的对象，而是通过 `DatabaseManager::instance()` 来获取全局唯一的实例。

**注意**: 目前为了方便测试，我们在客户端 (`FlightTicketClient`) 中也直接集成了这个模块。在最终版本中，客户端应该通过网络发送请求给服务器，由服务器调用这个模块。

## 2. 如何在代码中使用 (Usage)

### 2.1 包含头文件
在使用任何数据库功能之前，请在你的 `.cpp` 或 `.h` 文件顶部添加：
```cpp
#include "DatabaseManager.h"
```
*(注意：如果编译器提示找不到文件，请检查 CMakeLists.txt 是否正确包含了服务器端的路径)*

### 2.2 初始化 (Initialization)
程序启动时（通常在 `main.cpp` 或 `MainWindow` 构造函数中），必须先连接数据库：
```cpp
if (DatabaseManager::instance().connect("flight_system.db")) {
    qDebug() << "数据库连接成功！";
} else {
    qDebug() << "数据库连接失败！";
}
```
这会自动在程序运行目录下生成一个 `flight_system.db` 文件。

## 3. 数据结构 (Data Structures)
为了方便传递数据，我们定义了几个简单的结构体。

### 3.1 用户 (User)
```cpp
struct User {
    int id;             // 用户唯一ID (数据库自动生成)
    QString username;   // 用户名 (唯一)
    QString password;   // 密码
    QString phone;      // 电话号码
};
```

### 3.2 航班 (Flight)
```cpp
struct Flight {
    int id;                 // 航班唯一ID
    QString flightNumber;   // 航班号 (如 CA1234)
    QString origin;         // 出发地 (如 Beijing)
    QString destination;    // 目的地 (如 Shanghai)
    QDateTime departureTime;// 起飞时间
    QDateTime arrivalTime;  // 到达时间
    double price;           // 价格
    int totalSeats;         // 总座位数
    int availableSeats;     // 剩余座位数
};
```

### 3.3 订单 (Order)
```cpp
struct Order {
    int id;                 // 订单ID
    int userId;             // 下单用户ID
    int flightId;           // 预订航班ID
    QDateTime orderTime;    // 下单时间
    QString status;         // 订单状态 (booked: 已预订, cancelled: 已取消)
};
```

## 4. 详细接口说明 (API Details)

### 4.1 用户相关

#### 注册 (Register)
```cpp
bool registerUser(const QString& username, const QString& password, const QString& phone);
```
*   **作用**: 创建一个新用户。
*   **参数**:
    *   `username`: 用户名 (不能与现有用户重复)
    *   `password`: 密码
    *   `phone`: 电话
*   **返回值**: 注册成功返回 `true`，失败返回 `false`。

#### 登录 (Login)
```cpp
bool loginUser(const QString& username, const QString& password, User& userInfo);
```
*   **作用**: 验证账号密码。
*   **参数**:
    *   `username`: 用户名
    *   `password`: 密码
    *   `userInfo`: **(输出参数)** 如果登录成功，这个变量会被填充上用户的详细信息（包括ID）。
*   **返回值**: 登录成功返回 `true`，失败返回 `false`。

### 4.2 航班相关

#### 添加航班 (Add Flight)
```cpp
bool addFlight(const Flight& flight);
```
*   **作用**: 向数据库添加一条新的航班记录。
*   **参数**: 一个填充好数据的 `Flight` 对象。
*   **返回值**: 成功 `true`，失败 `false`。

#### 查询航班 (Query Flights)
```cpp
QList<Flight> queryFlights(const QString& origin, const QString& destination, const QDate& date);
```
*   **作用**: 根据条件搜索航班。
*   **参数**:
    *   `origin`: 出发地 (留空表示不限制)
    *   `destination`: 目的地 (留空表示不限制)
    *   `date`: 出发日期 (如果传入 `QDate()` 空日期，则表示不限制日期)
*   **返回值**: 返回一个 `QList<Flight>` 列表，包含所有符合条件的航班。

### 4.3 订单相关

#### 创建订单 (Create Order)
```cpp
bool createOrder(int userId, int flightId);
```
*   **作用**: 用户预订某个航班。
*   **逻辑**: 这个函数非常智能，它会开启一个**事务 (Transaction)**：
    1.  先检查该航班还有没有余票。
    2.  如果有，把余票减 1。
    3.  在订单表中创建一条新记录。
    4.  如果任何一步失败（比如没票了），它会自动回滚，取消所有操作。
*   **参数**: 用户ID, 航班ID。
*   **返回值**: 预订成功 `true`，失败 `false`。

#### 获取我的订单 (Get User Orders)
```cpp
QList<Order> getUserOrders(int userId);
```
*   **作用**: 查看某个用户的所有订单。
*   **参数**: 用户ID。
*   **返回值**: 订单列表。

## 5. 示例代码 (Example)

假设你在编写客户端的“预订按钮”点击事件：

```cpp
void onBookButtonClicked() {
    // 假设当前登录用户的ID是 1，想预订的航班ID是 5
    int currentUserId = 1;
    int flightIdToBook = 5;

    bool success = DatabaseManager::instance().createOrder(currentUserId, flightIdToBook);

    if (success) {
        QMessageBox::information(this, "提示", "预订成功！");
    } else {
        QMessageBox::warning(this, "提示", "预订失败，可能是票卖完了。");
    }
}
```

