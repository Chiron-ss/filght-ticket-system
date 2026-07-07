#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QtCore/QString>
#include <QtSql/QSqlDatabase>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QDate>
#include <QtSql/QSqlQuery>
#include <QtCore/QVariant>

// 用户信息结构体
struct User {
    int id;
    QString username;
    QString password;
    QString phone;
    QDateTime createTime;

    User() : id(-1) {}
};

// 航班信息结构体
struct Flight {
    int id;
    QString flightNumber;
    QString origin;
    QString destination;
    QDateTime departureTime;
    QDateTime arrivalTime;
    QString airline;
    double economyPrice;     // 经济舱价格
    double businessPrice;    // 公务舱价格
    int totalEconomySeats;   // 经济舱总座位数
    int availableEconomySeats; // 经济舱可用座位数
    int totalBusinessSeats;  // 公务舱总座位数
    int availableBusinessSeats; // 公务舱可用座位数
    QString aircraftModel;   // 机型
    bool hasMeal;           // 是否含有餐食

    Flight() : id(-1), economyPrice(0.0), businessPrice(0.0),
        totalEconomySeats(0), availableEconomySeats(0),
        totalBusinessSeats(0), availableBusinessSeats(0),
        hasMeal(false) {}
};

// 订单信息结构体
struct Order {
    int id;
    int userId;
    int flightId;
    QString seatClass;      // 舱位类型：economy/business
    QString seatNumber;     // 座位号
    QString gate;           // 登机口
    QDateTime orderTime;
    QString status;         // 状态：booked, traveling, completed, cancelled

    Order() : id(-1), userId(-1), flightId(-1) {}
};

class DatabaseManager
{
public:
    static DatabaseManager& instance();

    // 数据库连接管理
    bool connect(const QString& path = "flight_system.db");
    bool isConnected() const;

    // 用户管理
    bool registerUser(const QString& username, const QString& password, const QString& phone);
    bool loginUser(const QString& username, const QString& password, User& userInfo);
    User getUserById(int userId);
    bool isUsernameExists(const QString& username);

    // 航班管理
    bool addFlight(const Flight& flight);
    Flight getFlightById(int flightId);
    QList<Flight> queryFlights(const QString& origin = "", const QString& destination = "", const QDate& date = QDate());
    QList<Flight> getAllFlights();

    // 座位管理函数
    bool updateSeatAvailability(int flightId, int seatChange, const QString& seatClass);
    int getAvailableSeats(int flightId, const QString& seatClass);
    bool hasAvailableSeats(int flightId, int requiredSeats, const QString& seatClass);

    // 订单管理
    bool createOrder(int userId, int flightId, const QString& seatClass);
    Order getOrderById(int orderId);
    QList<Order> getUserOrders(int userId);
    bool cancelOrder(int orderId);

    // 数据维护
    bool clearTestData();
    bool addTestFlights();

    // 统计功能
    int getFlightCount();
    int getUserCount();
    int getOrderCount();

private:
    DatabaseManager();
    ~DatabaseManager();

    // 禁止拷贝
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool initDatabase();
    bool createTables();
    bool checkTableStructure();
    bool createUserTable();
    bool createFlightTable();
    bool createOrderTable();

    // 事务支持
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H
