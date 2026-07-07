#include "DatabaseManager.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtSql/QSqlError>
#include <QtCore/QDateTime>
#include <QtCore/QTime>
#include <QtCore/QRandomGenerator>

// 单例实例
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager()
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::connect(const QString& path)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qDebug() << "数据库连接失败:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "数据库连接成功";
    return initDatabase();
}

bool DatabaseManager::isConnected() const
{
    return m_db.isOpen();
}

bool DatabaseManager::initDatabase()
{


    // 初始化后创建新表
    return createTables();
}

bool DatabaseManager::createTables()
{
    return createUserTable() && createFlightTable() && createOrderTable();
}

bool DatabaseManager::checkTableStructure()
{
    QSqlQuery query;

    // 检查flights表结构
    if (query.exec("PRAGMA table_info(flights)")) {
        qDebug() << "=== flights表结构 ===";
        while (query.next()) {
            QString columnName = query.value("name").toString();
            QString columnType = query.value("type").toString();
            qDebug() << "列:" << columnName << "类型:" << columnType;
        }
    }

    return true;
}
bool DatabaseManager::createUserTable()
{
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username VARCHAR(50) UNIQUE NOT NULL,
            password VARCHAR(100) NOT NULL,
            phone VARCHAR(20),
            create_time DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    QSqlQuery query;
    if (!query.exec(sql)) {
        qDebug() << "创建用户表失败:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createFlightTable()
{
    // 先删除已存在的表（开发环境使用）
    QSqlQuery dropQuery("DROP TABLE IF EXISTS flights");
    dropQuery.exec();

    QString sql = R"(
        CREATE TABLE flights (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            flight_number VARCHAR(20) NOT NULL,
            origin VARCHAR(50) NOT NULL,
            destination VARCHAR(50) NOT NULL,
            departure_time DATETIME,
            arrival_time DATETIME,
            airline VARCHAR(50),
            economy_price DECIMAL(10,2) NOT NULL DEFAULT 0,
            business_price DECIMAL(10,2) NOT NULL DEFAULT 0,
            total_economy_seats INTEGER NOT NULL DEFAULT 0,
            available_economy_seats INTEGER NOT NULL DEFAULT 0,
            total_business_seats INTEGER NOT NULL DEFAULT 0,
            available_business_seats INTEGER NOT NULL DEFAULT 0,
            aircraft_model VARCHAR(50),
            has_meal BOOLEAN DEFAULT 0,
            created_time DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    QSqlQuery query;
    if (!query.exec(sql)) {
        qDebug() << "创建航班表失败:" << query.lastError().text();
        return false;
    }

    qDebug() << "航班表创建成功";
    return true;
}

bool DatabaseManager::createOrderTable()
{
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            flight_id INTEGER NOT NULL,
            seat_class VARCHAR(20) NOT NULL,
            seat_number VARCHAR(10),
            gate VARCHAR(10),
            order_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            status VARCHAR(20) DEFAULT 'booked',
            FOREIGN KEY(user_id) REFERENCES users(id),
            FOREIGN KEY(flight_id) REFERENCES flights(id)
        )
    )";

    QSqlQuery query;
    if (!query.exec(sql)) {
        qDebug() << "创建订单表失败:" << query.lastError().text();
        return false;
    }
    return true;
}

// ==================== 用户管理 ====================
bool DatabaseManager::registerUser(const QString& username, const QString& password, const QString& phone)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, phone) VALUES (?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(password);
    query.addBindValue(phone);

    if (!query.exec()) {
        qDebug() << "注册错误:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::loginUser(const QString& username, const QString& password, User& userInfo)
{
    QSqlQuery query;
    query.prepare("SELECT id, username, password, phone, create_time FROM users WHERE username = ? AND password = ?");
    query.addBindValue(username);
    query.addBindValue(password);

    if (query.exec() && query.next()) {
        userInfo.id = query.value("id").toInt();
        userInfo.username = query.value("username").toString();
        userInfo.password = query.value("password").toString();
        userInfo.phone = query.value("phone").toString();
        userInfo.createTime = query.value("create_time").toDateTime();
        return true;
    }
    return false;
}

User DatabaseManager::getUserById(int userId)
{
    User user;
    user.id = -1;

    QSqlQuery query;
    query.prepare("SELECT id, username, password, phone, create_time FROM users WHERE id = ?");
    query.addBindValue(userId);

    if (query.exec() && query.next()) {
        user.id = query.value("id").toInt();
        user.username = query.value("username").toString();
        user.password = query.value("password").toString();
        user.phone = query.value("phone").toString();
        user.createTime = query.value("create_time").toDateTime();
    }
    return user;
}

bool DatabaseManager::isUsernameExists(const QString& username)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username = ?");
    query.addBindValue(username);
    return query.exec() && query.next();
}

// ==================== 航班管理 ====================
bool DatabaseManager::addFlight(const Flight& flight)
{
    QSqlQuery query;
    query.prepare("INSERT INTO flights (flight_number, origin, destination, departure_time, arrival_time, airline, economy_price, business_price, total_economy_seats, available_economy_seats, total_business_seats, available_business_seats, aircraft_model, has_meal) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(flight.flightNumber);
    query.addBindValue(flight.origin);
    query.addBindValue(flight.destination);
    query.addBindValue(flight.departureTime);
    query.addBindValue(flight.arrivalTime);
    query.addBindValue(flight.airline);
    query.addBindValue(flight.economyPrice);
    query.addBindValue(flight.businessPrice);
    query.addBindValue(flight.totalEconomySeats);
    query.addBindValue(flight.availableEconomySeats);
    query.addBindValue(flight.totalBusinessSeats);
    query.addBindValue(flight.availableBusinessSeats);
    query.addBindValue(flight.aircraftModel);
    query.addBindValue(flight.hasMeal);

    if (!query.exec()) {
        qDebug() << "添加航班失败:" << query.lastError().text();
        qDebug() << "SQL错误:" << query.lastQuery();
        return false;
    }

    qDebug() << "成功添加航班:" << flight.flightNumber;
    return true;
}

Flight DatabaseManager::getFlightById(int flightId)
{
    Flight flight;
    flight.id = -1;

    QSqlQuery query;
    query.prepare("SELECT * FROM flights WHERE id = ?");
    query.addBindValue(flightId);

    if (query.exec() && query.next()) {
        flight.id = query.value("id").toInt();
        flight.flightNumber = query.value("flight_number").toString();
        flight.origin = query.value("origin").toString();
        flight.destination = query.value("destination").toString();
        flight.departureTime = query.value("departure_time").toDateTime();
        flight.arrivalTime = query.value("arrival_time").toDateTime();
        flight.airline = query.value("airline").toString();
        flight.economyPrice = query.value("economy_price").toDouble();
        flight.businessPrice = query.value("business_price").toDouble();
        flight.totalEconomySeats = query.value("total_economy_seats").toInt();
        flight.availableEconomySeats = query.value("available_economy_seats").toInt();
        flight.totalBusinessSeats = query.value("total_business_seats").toInt();
        flight.availableBusinessSeats = query.value("available_business_seats").toInt();
        flight.aircraftModel = query.value("aircraft_model").toString();
        flight.hasMeal = query.value("has_meal").toBool();
    } else {
        qDebug() << "查询航班失败:" << query.lastError().text();
    }

    return flight;
}

QList<Flight> DatabaseManager::queryFlights(const QString& origin, const QString& destination, const QDate& date)
{
    QList<Flight> flights;
    QString sql = "SELECT * FROM flights WHERE 1=1";

    QList<QVariant> params;

    if (!origin.isEmpty()) {
        sql += " AND origin LIKE ?";
        params.append("%" + origin + "%");
    }
    if (!destination.isEmpty()) {
        sql += " AND destination LIKE ?";
        params.append("%" + destination + "%");
    }
    if (date.isValid()) {
        sql += " AND DATE(departure_time) = ?";
        params.append(date.toString("yyyy-MM-dd"));
    }

    QSqlQuery query;
    query.prepare(sql);

    for (int i = 0; i < params.size(); ++i) {
        query.addBindValue(params[i]);
    }

    if (!query.exec()) {
        qDebug() << "查询航班失败:" << query.lastError().text();
        return flights;
    }

    while (query.next()) {
        Flight flight;
        flight.id = query.value("id").toInt();
        flight.flightNumber = query.value("flight_number").toString();
        flight.origin = query.value("origin").toString();
        flight.destination = query.value("destination").toString();
        flight.departureTime = query.value("departure_time").toDateTime();
        flight.arrivalTime = query.value("arrival_time").toDateTime();
        flight.airline = query.value("airline").toString();
        flight.economyPrice = query.value("economy_price").toDouble();
        flight.businessPrice = query.value("business_price").toDouble();
        flight.totalEconomySeats = query.value("total_economy_seats").toInt();
        flight.availableEconomySeats = query.value("available_economy_seats").toInt();
        flight.totalBusinessSeats = query.value("total_business_seats").toInt();
        flight.availableBusinessSeats = query.value("available_business_seats").toInt();
        flight.aircraftModel = query.value("aircraft_model").toString();
        flight.hasMeal = query.value("has_meal").toBool();
        flights.append(flight);
    }

    return flights;
}

QList<Flight> DatabaseManager::getAllFlights()
{
    return queryFlights("", "", QDate());
}

// ==================== 座位管理 ====================
bool DatabaseManager::updateSeatAvailability(int flightId, int seatChange, const QString& seatClass)
{
    QString sql;
    if (seatClass == "business") {
        sql = "UPDATE flights SET available_business_seats = available_business_seats + ? WHERE id = ?";
    } else {
        sql = "UPDATE flights SET available_economy_seats = available_economy_seats + ? WHERE id = ?";
    }

    QSqlQuery query;
    query.prepare(sql);
    query.addBindValue(seatChange);
    query.addBindValue(flightId);

    if (!query.exec()) {
        qDebug() << "更新座位失败:" << query.lastError().text();
        return false;
    }
    return true;
}

int DatabaseManager::getAvailableSeats(int flightId, const QString& seatClass)
{
    QString sql;
    if (seatClass == "business") {
        sql = "SELECT available_business_seats FROM flights WHERE id = ?";
    } else {
        sql = "SELECT available_economy_seats FROM flights WHERE id = ?";
    }

    QSqlQuery query;
    query.prepare(sql);
    query.addBindValue(flightId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

bool DatabaseManager::hasAvailableSeats(int flightId, int requiredSeats, const QString& seatClass)
{
    int available = getAvailableSeats(flightId, seatClass);
    return available >= requiredSeats;
}

// ==================== 订单管理 ====================
bool DatabaseManager::createOrder(int userId, int flightId, const QString& seatClass)
{
    if (!beginTransaction()) {
        return false;
    }

    // 检查余票
    if (!hasAvailableSeats(flightId, 1, seatClass)) {
        rollbackTransaction();
        return false;
    }

    // 扣减余票
    if (!updateSeatAvailability(flightId, -1, seatClass)) {
        rollbackTransaction();
        return false;
    }

    // 生成随机座位号和登机口
    QString seatNumber = QString("%1%2").arg(QRandomGenerator::global()->bounded(1, 31)).arg(char('A' + QRandomGenerator::global()->bounded(6)));
    QString gate = QString("%1%2").arg(char('A' + QRandomGenerator::global()->bounded(5))).arg(QRandomGenerator::global()->bounded(1, 50));

    // 创建订单
    QSqlQuery query;
    query.prepare("INSERT INTO orders (user_id, flight_id, seat_class, seat_number, gate, status) VALUES (?, ?, ?, ?, ?, 'booked')");
    query.addBindValue(userId);
    query.addBindValue(flightId);
    query.addBindValue(seatClass);
    query.addBindValue(seatNumber);
    query.addBindValue(gate);

    if (!query.exec()) {
        rollbackTransaction();
        qDebug() << "创建订单失败:" << query.lastError().text();
        return false;
    }

    return commitTransaction();
}

Order DatabaseManager::getOrderById(int orderId)
{
    Order order;
    order.id = -1;

    QSqlQuery query;
    query.prepare("SELECT * FROM orders WHERE id = ?");
    query.addBindValue(orderId);

    if (query.exec() && query.next()) {
        order.id = query.value("id").toInt();
        order.userId = query.value("user_id").toInt();
        order.flightId = query.value("flight_id").toInt();
        order.seatClass = query.value("seat_class").toString();
        order.seatNumber = query.value("seat_number").toString();
        order.gate = query.value("gate").toString();
        order.orderTime = query.value("order_time").toDateTime();
        order.status = query.value("status").toString();
    }
    return order;
}

QList<Order> DatabaseManager::getUserOrders(int userId)
{
    QList<Order> orders;

    QSqlQuery query;
    query.prepare("SELECT * FROM orders WHERE user_id = ? ORDER BY order_time DESC");
    query.addBindValue(userId);

    if (query.exec()) {
        while (query.next()) {
            Order order;
            order.id = query.value("id").toInt();
            order.userId = query.value("user_id").toInt();
            order.flightId = query.value("flight_id").toInt();
            order.seatClass = query.value("seat_class").toString();
            order.seatNumber = query.value("seat_number").toString();
            order.gate = query.value("gate").toString();
            order.orderTime = query.value("order_time").toDateTime();
            order.status = query.value("status").toString();
            orders.append(order);
        }
    }

    return orders;
}

bool DatabaseManager::cancelOrder(int orderId)
{
    if (!beginTransaction()) {
        return false;
    }

    // 获取订单信息
    Order order = getOrderById(orderId);
    if (order.id == -1) {
        rollbackTransaction();
        return false;
    }

    // 恢复余票
    if (!updateSeatAvailability(order.flightId, 1, order.seatClass)) {
        rollbackTransaction();
        return false;
    }

    // 更新订单状态
    QSqlQuery query;
    query.prepare("UPDATE orders SET status = 'cancelled' WHERE id = ?");
    query.addBindValue(orderId);

    if (!query.exec()) {
        rollbackTransaction();
        return false;
    }

    return commitTransaction();
}

// ==================== 数据维护 ====================
bool DatabaseManager::clearTestData()
{
    if (!beginTransaction()) {
        return false;
    }

    QSqlQuery query1("DELETE FROM orders");
    QSqlQuery query2("DELETE FROM flights");
    QSqlQuery query3("DELETE FROM users");

    if (query1.exec() && query2.exec() && query3.exec()) {
        return commitTransaction();
    } else {
        rollbackTransaction();
        return false;
    }
}

bool DatabaseManager::addTestFlights()
{
    // 清理旧数据
    clearTestData();

    QList<Flight> testFlights;
    QDateTime now = QDateTime::currentDateTime();

    // 热门城市
    QStringList cities = {"北京", "上海", "广州", "深圳", "成都", "杭州", "西安", "昆明", "南京", "重庆"};
    QStringList airlines = {"中国国航", "东方航空", "南方航空", "海南航空", "厦门航空", "深圳航空", "四川航空", "春秋航空"};
    QStringList models = {"波音737-800", "波音787-9", "空客A320", "空客A330", "空客A350", "C919"};

    // 生成未来30天的航班
    for (int i = 0; i < 30; ++i) {
        QDate date = now.date().addDays(i);
        
        // 每天生成5-10个航班
        int dailyFlights = 5 + (QRandomGenerator::global()->bounded(6));
        for (int j = 0; j < dailyFlights; ++j) {
            Flight f;
            int originIdx = QRandomGenerator::global()->bounded(cities.size());
            int destIdx = (originIdx + 1 + (QRandomGenerator::global()->bounded(cities.size() - 1))) % cities.size();
            
            f.origin = cities[originIdx];
            f.destination = cities[destIdx];
            f.flightNumber = QString("%1%2").arg(char('A' + (QRandomGenerator::global()->bounded(26)))).arg(1000 + (QRandomGenerator::global()->bounded(9000)));
            
            int hour = 6 + (QRandomGenerator::global()->bounded(16)); // 6:00 to 22:00
            int minute = (QRandomGenerator::global()->bounded(12)) * 5; // 0, 5, 10... 55
            f.departureTime = QDateTime(date, QTime(hour, minute));
            
            int duration = 90 + (QRandomGenerator::global()->bounded(180)); // 1.5 to 4.5 hours
            f.arrivalTime = f.departureTime.addSecs(duration * 60);
            
            f.airline = airlines[QRandomGenerator::global()->bounded(airlines.size())];
            f.aircraftModel = models[QRandomGenerator::global()->bounded(models.size())];
            
            f.economyPrice = 400 + (QRandomGenerator::global()->bounded(1200));
            f.businessPrice = f.economyPrice * (2.0 + (QRandomGenerator::global()->bounded(10)) / 10.0);
            
            f.totalEconomySeats = 150;
            f.availableEconomySeats = QRandomGenerator::global()->bounded(151);
            f.totalBusinessSeats = 20;
            f.availableBusinessSeats = QRandomGenerator::global()->bounded(21);
            
            f.hasMeal = (QRandomGenerator::global()->bounded(2) == 0);
            
            testFlights.append(f);
        }
    }

    // 添加到数据库
    int successCount = 0;
    for (const Flight& flight : testFlights) {
        if (addFlight(flight)) {
            successCount++;
        }
    }

    qDebug() << "总计添加:" << successCount << "/" << testFlights.size() << "个航班";
    return successCount > 0;
}

// ==================== 事务支持 ====================
bool DatabaseManager::beginTransaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    return m_db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    return m_db.rollback();
}

// ==================== 统计功能 ====================
int DatabaseManager::getFlightCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM flights");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getUserCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM users");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getOrderCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM orders");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
