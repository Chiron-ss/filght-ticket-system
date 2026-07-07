#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include "adminmainwindow.h"

#include <QtWidgets/QMessageBox>
#include <QtCore/QDateTime>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QListWidgetItem>
#include <QtCore/QDebug>
#include <QtWidgets/QLabel>
#include <QtGui/QBrush>
#include <QtGui/QFont>
#include <algorithm>
#include <utility>
#include <QtWidgets/QRadioButton>
#include <QtCore/QDate>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentUser(nullptr)
    , m_isLoggedIn(false)
    , m_resultLabel(nullptr)
{
    ui->setupUi(this);

    // 设置窗口属性
    setWindowTitle("航班票务管理系统");
    resize(1200, 800);

    // 初始化结果标签
    m_resultLabel = new QLabel("找到0个航班");
    ui->statusbar->addPermanentWidget(m_resultLabel);

    // 初始化提醒定时器
    m_reminderTimer = new QTimer(this);
    connect(m_reminderTimer, &QTimer::timeout, this, &MainWindow::checkFlightReminders);
    m_reminderTimer->start(60000); // 每分钟检查一次

    // 初始化时间控件
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime startOfDay = QDateTime(currentDateTime.date(), QTime(0, 0, 0));
    QDateTime endOfDay = QDateTime(currentDateTime.date().addDays(30), QTime(23, 59, 59));

    ui->dateTimeEdit_from->setDateTime(startOfDay);
    ui->dateTimeEdit_to->setDateTime(endOfDay);

    // 添加管理员入口按钮
    QPushButton* btnAdmin = new QPushButton("管理员后台", this);
    btnAdmin->setObjectName("btn_admin_entry");
    btnAdmin->setMinimumWidth(120);
    ui->statusbar->addPermanentWidget(btnAdmin);
    connect(btnAdmin, &QPushButton::clicked, this, &MainWindow::on_btn_admin_clicked);

    // 初始化数据库连接
    QString dbPath = QCoreApplication::applicationDirPath() + "/flight_system.db";
    if (!DatabaseManager::instance().connect(dbPath)) {
        QMessageBox::critical(this, "错误", "数据库连接失败！");
        return;
    }

    // 添加测试数据
    if (!DatabaseManager::instance().addTestFlights()) {
        qDebug() << "添加测试数据失败";
    } else {
        qDebug() << "测试数据添加成功";
    }

    // 初始化界面组件
    initFlightTable();
    initConnections();
    updateUserInterface();

    // 初始显示所有航班
    on_btn_query_clicked();

    showStatusMessage("系统初始化完成，测试数据已加载", 3000);
}

MainWindow::~MainWindow()
{
    delete m_currentUser;
    delete ui;
}

void MainWindow::initFlightTable()
{
    // 设置表格列数和标题 - 添加舱位类型、机型、餐食列
    ui->table_flights->setColumnCount(13);
    QStringList headers = {"ID", "航班号", "出发地", "目的地", "起飞时间", "到达时间", "航空公司", "经济舱价格", "公务舱价格", "经济舱余票", "公务舱余票", "机型", "餐食"};
    ui->table_flights->setHorizontalHeaderLabels(headers);

    // 设置表格属性
    ui->table_flights->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table_flights->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table_flights->setAlternatingRowColors(true);
    ui->table_flights->setSortingEnabled(false);

    // 设置列宽
    ui->table_flights->setColumnWidth(0, 50);
    ui->table_flights->setColumnWidth(1, 100);
    ui->table_flights->setColumnWidth(2, 80);
    ui->table_flights->setColumnWidth(3, 80);
    ui->table_flights->setColumnWidth(4, 150);
    ui->table_flights->setColumnWidth(5, 150);
    ui->table_flights->setColumnWidth(6, 120);
    ui->table_flights->setColumnWidth(7, 100);
    ui->table_flights->setColumnWidth(8, 100);
    ui->table_flights->setColumnWidth(9, 100);
    ui->table_flights->setColumnWidth(10, 100);
    ui->table_flights->setColumnWidth(11, 100);
    ui->table_flights->setColumnWidth(12, 60);
}

void MainWindow::initConnections()
{
    // 方案一：依赖Qt的自动连接，此函数内容可为空。
    // 所有以 on_btnName_clicked() 命名的槽函数已被自动连接。
    qDebug() << "信号槽连接采用Qt自动连接机制";
}

void MainWindow::on_btn_admin_clicked()
{
    // 弹出管理员登录对话框
    bool ok;
    QString password = QInputDialog::getText(this, "管理员验证", "请输入管理员密码：", QLineEdit::Password, "", &ok);
    
    if (ok && password == "admin123") {
        AdminMainWindow* adminWin = new AdminMainWindow();
        adminWin->setAttribute(Qt::WA_DeleteOnClose);
        adminWin->adminLogin("admin", "admin123"); // 自动登录
        adminWin->show();
    } else if (ok) {
        QMessageBox::warning(this, "验证失败", "密码错误！");
    }
}

// ==================== 用户管理槽函数实现 ====================
void MainWindow::on_btn_register_clicked()
{
    QString username = ui->lineEdit_username->text().trimmed();
    QString password = ui->lineEdit_password->text();
    QString phone = ui->lineEdit_phone->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || phone.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "所有字段都不能为空");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "输入错误", "密码长度不能少于6位");
        return;
    }

    bool success = DatabaseManager::instance().registerUser(username, password, phone);

    if (success) {
        ui->lineEdit_username->clear();
        ui->lineEdit_password->clear();
        ui->lineEdit_phone->clear();
        QMessageBox::information(this, "成功", "注册成功！");
        showStatusMessage("注册成功", 2000);
    } else {
        QMessageBox::warning(this, "失败", "注册失败，用户名可能已存在");
        showStatusMessage("注册失败", 2000);
    }
}

void MainWindow::on_btn_login_clicked()
{
    QString username = ui->lineEdit_username->text().trimmed();
    QString password = ui->lineEdit_password->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "用户名和密码不能为空");
        return;
    }

    User userInfo;
    if (DatabaseManager::instance().loginUser(username, password, userInfo)) {
        // 删除旧的用户对象
        if (m_currentUser) {
            delete m_currentUser;
        }

        // 创建新的用户对象
        m_currentUser = new User();
        m_currentUser->id = userInfo.id;
        m_currentUser->username = userInfo.username;
        m_currentUser->password = userInfo.password;
        m_currentUser->phone = userInfo.phone;
        m_currentUser->createTime = userInfo.createTime;

        m_isLoggedIn = true;

        QMessageBox::information(this, "登录成功", QString("欢迎回来，%1！").arg(username));
        updateUserInterface();
        ui->tabWidget->setCurrentIndex(1); // 切换到航班查询标签
        showStatusMessage(QString("欢迎回来，%1！").arg(username), 3000);

        // 登录成功后刷新订单列表
        on_btn_refresh_orders_clicked();
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
        showStatusMessage("登录失败", 2000);
    }
}

void MainWindow::on_btn_logout_clicked()
{
    if (!m_isLoggedIn) {
        QMessageBox::information(this, "提示", "您尚未登录");
        return;
    }

    // 清理用户数据
    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
    }
    m_isLoggedIn = false;

    updateUserInterface();
    ui->lineEdit_username->clear();
    ui->lineEdit_password->clear();
    ui->lineEdit_phone->clear();

    QMessageBox::information(this, "成功", "已成功退出登录");
    showStatusMessage("已退出登录", 2000);
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::on_btn_clear_input_clicked()
{
    ui->lineEdit_username->clear();
    ui->lineEdit_password->clear();
    ui->lineEdit_phone->clear();
    showStatusMessage("输入框已清空", 1500);
}

// ==================== 航班查询槽函数实现 ====================
void MainWindow::on_btn_query_clicked()
{
    QString origin = ui->lineEdit_origin->text().trimmed();
    QString destination = ui->lineEdit_dest->text().trimmed();

    // 获取时间范围
    QDateTime fromTime = ui->dateTimeEdit_from->dateTime();
    QDateTime toTime = ui->dateTimeEdit_to->dateTime();

    // 查询所有航班
    QList<Flight> allFlights = DatabaseManager::instance().getAllFlights();

    // 在内存中过滤结果
    QList<Flight> filteredFlights;
    for (const Flight& flight : std::as_const(allFlights)) {
        bool matches = true;

        // 检查出发地
        if (!origin.isEmpty() && !flight.origin.contains(origin)) {
            matches = false;
        }

        // 检查目的地
        if (!destination.isEmpty() && !flight.destination.contains(destination)) {
            matches = false;
        }

        // 检查时间范围
        if (flight.departureTime < fromTime || flight.departureTime > toTime) {
            matches = false;
        }

        if (matches) {
            filteredFlights.append(flight);
        }
    }

    // 根据排序选项排序
    if (ui->radioButton_price_high_low->isChecked()) {
        std::sort(filteredFlights.begin(), filteredFlights.end(),
                  [](const Flight& a, const Flight& b) { return a.economyPrice > b.economyPrice; });
    } else if (ui->radioButton_by_seats->isChecked()) {
        std::sort(filteredFlights.begin(), filteredFlights.end(),
                  [](const Flight& a, const Flight& b) { return a.availableEconomySeats > b.availableEconomySeats; });
    } else {
        // 默认按经济舱价格从低到高
        std::sort(filteredFlights.begin(), filteredFlights.end(),
                  [](const Flight& a, const Flight& b) { return a.economyPrice < b.economyPrice; });
    }

    // 显示结果
    displayFlights(filteredFlights);

    // 更新状态信息
    QString status = QString("找到 %1 个航班").arg(filteredFlights.size());
    if (!origin.isEmpty() || !destination.isEmpty()) {
        status += QString(" (条件: %1→%2)").arg(origin.isEmpty() ? "任意" : origin,
                                               destination.isEmpty() ? "任意" : destination);
    }

    // 添加时间范围信息
    status += QString(" 时间: %1 - %2")
                  .arg(fromTime.toString("MM/dd hh:mm"), toTime.toString("MM/dd hh:mm"));

    showStatusMessage(status, 5000);
}

void MainWindow::on_btn_add_test_flight_clicked()
{
    // 先清空现有数据
    DatabaseManager::instance().clearTestData();

    // 创建测试航班数据
    QList<Flight> testFlights;

    // 航班1
    Flight flight1;
    flight1.id = 1;
    flight1.flightNumber = "CA1234";
    flight1.origin = "北京";
    flight1.destination = "上海";
    flight1.departureTime = QDateTime(QDate(2024, 3, 1), QTime(8, 30));
    flight1.arrivalTime = QDateTime(QDate(2024, 3, 1), QTime(10, 45));
    flight1.airline = "中国国际航空";
    flight1.economyPrice = 680.0;
    flight1.businessPrice = 1280.0;
    flight1.totalEconomySeats = 150;
    flight1.availableEconomySeats = 45;
    flight1.totalBusinessSeats = 30;
    flight1.availableBusinessSeats = 12;
    flight1.aircraftModel = "波音737";
    flight1.hasMeal = true;

    // 航班2
    Flight flight2;
    flight2.id = 2;
    flight2.flightNumber = "MU5678";
    flight2.origin = "上海";
    flight2.destination = "广州";
    flight2.departureTime = QDateTime(QDate(2024, 3, 1), QTime(14, 20));
    flight2.arrivalTime = QDateTime(QDate(2024, 3, 1), QTime(16, 50));
    flight2.airline = "东方航空";
    flight2.economyPrice = 720.0;
    flight2.businessPrice = 1320.0;
    flight2.totalEconomySeats = 120;
    flight2.availableEconomySeats = 32;
    flight2.totalBusinessSeats = 25;
    flight2.availableBusinessSeats = 8;
    flight2.aircraftModel = "空客A320";
    flight2.hasMeal = true;

    // 航班3
    Flight flight3;
    flight3.id = 3;
    flight3.flightNumber = "SC1234";
    flight3.origin = "成都";
    flight3.destination = "北京";
    flight3.departureTime = QDateTime(QDate(2024, 3, 2), QTime(9, 15));
    flight3.arrivalTime = QDateTime(QDate(2024, 3, 2), QTime(11, 40));
    flight3.airline = "山东航空";
    flight3.economyPrice = 780.0;
    flight3.businessPrice = 1480.0;
    flight3.totalEconomySeats = 100;
    flight3.availableEconomySeats = 7;
    flight3.totalBusinessSeats = 20;
    flight3.availableBusinessSeats = 3;
    flight3.aircraftModel = "波音777";
    flight3.hasMeal = false;

    testFlights.append(flight1);
    testFlights.append(flight2);
    testFlights.append(flight3);

    // 添加到数据库
    int successCount = 0;
    for (const Flight& flight : std::as_const(testFlights)) {
        if (DatabaseManager::instance().addFlight(flight)) {
            successCount++;
        }
    }

    // 刷新显示
    on_btn_query_clicked();

    QMessageBox::information(this, "提示",
                             QString("成功添加 %1 条测试航班数据！").arg(successCount));
    showStatusMessage(QString("已添加 %1 个测试航班").arg(successCount), 3000);
}

// ==================== 排序槽函数实现 ====================
void MainWindow::on_radioButton_price_low_high_clicked()
{
    sortFlightsByPrice(true);
}

void MainWindow::on_radioButton_price_high_low_clicked()
{
    sortFlightsByPrice(false);
}

void MainWindow::on_radioButton_by_seats_clicked()
{
    sortFlightsBySeats();
}

void MainWindow::sortFlightsByPrice(bool ascending)
{
    QString origin = ui->lineEdit_origin->text().trimmed();
    QString destination = ui->lineEdit_dest->text().trimmed();
    QList<Flight> flights = DatabaseManager::instance().queryFlights(origin, destination, QDate());

    // 按经济舱价格排序
    std::sort(flights.begin(), flights.end(), [ascending](const Flight& a, const Flight& b) {
        return ascending ? (a.economyPrice < b.economyPrice) : (a.economyPrice > b.economyPrice);
    });

    displayFlights(flights);
    showStatusMessage(QString("已按经济舱价格%1排序").arg(ascending ? "从低到高" : "从高到低"), 2000);
}

void MainWindow::sortFlightsBySeats()
{
    QString origin = ui->lineEdit_origin->text().trimmed();
    QString destination = ui->lineEdit_dest->text().trimmed();
    QList<Flight> flights = DatabaseManager::instance().queryFlights(origin, destination, QDate());

    // 按经济舱余票排序
    std::sort(flights.begin(), flights.end(), [](const Flight& a, const Flight& b) {
        return a.availableEconomySeats > b.availableEconomySeats;
    });

    displayFlights(flights);
    showStatusMessage("已按经济舱余票从多到少排序", 2000);
}


// ==================== 订单管理槽函数实现 ====================
void MainWindow::on_btn_quick_book_clicked()
{
    if (!m_isLoggedIn || !m_currentUser) {
        QMessageBox::warning(this, "警告", "请先登录");
        ui->tabWidget->setCurrentIndex(0);
        return;
    }

    int currentRow = ui->table_flights->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请先选择一个航班");
        return;
    }

    QTableWidgetItem* idItem = ui->table_flights->item(currentRow, 0);
    if (!idItem) {
        QMessageBox::warning(this, "错误", "无法获取航班信息");
        return;
    }

    int flightId = idItem->text().toInt();

    // 获取航班详情
    Flight flight = DatabaseManager::instance().getFlightById(flightId);
    if (flight.id == -1) {
        QMessageBox::warning(this, "错误", "航班不存在");
        return;
    }

    // 让用户选择舱位类型
    QMessageBox msgBox;
    msgBox.setWindowTitle("选择舱位");
    msgBox.setText("请选择舱位类型：");
    QPushButton *economyButton = msgBox.addButton("经济舱", QMessageBox::ActionRole);
    QPushButton *businessButton = msgBox.addButton("公务舱", QMessageBox::ActionRole);
    msgBox.addButton("取消", QMessageBox::RejectRole);

    msgBox.exec();

    QString seatClass;
    double price = 0.0;

    if (msgBox.clickedButton() == economyButton) {
        seatClass = "economy";
        price = flight.economyPrice;
    } else if (msgBox.clickedButton() == businessButton) {
        seatClass = "business";
        price = flight.businessPrice;
    } else {
        return; // 用户取消
    }

    // 检查余票
    int availableSeats = DatabaseManager::instance().getAvailableSeats(flightId, seatClass);
    if (availableSeats <= 0) {
        QMessageBox::warning(this, "预订失败", QString("%1舱位已无余票").arg(seatClass == "economy" ? "经济舱" : "公务舱"));
        return;
    }

    int reply = QMessageBox::question(this, "确认预订",
                                      QString("确认预订航班 %1?\n%2 → %3\n舱位: %4\n价格: %5 元")
                                          .arg(flight.flightNumber, flight.origin, flight.destination,
                                               seatClass == "economy" ? "经济舱" : "公务舱", QString::number(price)));

    if (reply == QMessageBox::Yes) {
        if (DatabaseManager::instance().createOrder(m_currentUser->id, flightId, seatClass)) {
            QMessageBox::information(this, "成功", "预订成功！");
            showStatusMessage("预订成功", 2000);
            on_btn_query_clicked(); // 刷新航班列表
            on_btn_refresh_orders_clicked(); // 刷新订单列表
        } else {
            QMessageBox::warning(this, "失败", "预订失败（可能是余票不足）");
            showStatusMessage("预订失败", 2000);
        }
    }
}



// 取消选中订单
void MainWindow::on_btn_cancel_order_clicked()
{
    if (!m_isLoggedIn || !m_currentUser) {
        QMessageBox::warning(this, "警告", "请先登录");
        return;
    }

    QListWidgetItem* currentItem = ui->list_orders->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "警告", "请先选择一个订单");
        return;
    }

    // 从订单文本中提取订单ID
    QString itemText = currentItem->text();
    QStringList parts = itemText.split("|");
    if (parts.size() < 1) {
        QMessageBox::warning(this, "错误", "订单格式错误");
        return;
    }

    QString orderIdStr = parts[0].split(":")[1].trimmed();
    int orderId = orderIdStr.toInt();

    int reply = QMessageBox::question(this, "确认取消",
                                      "确定要取消这个订单吗？");

    if (reply == QMessageBox::Yes) {
        if (DatabaseManager::instance().cancelOrder(orderId)) {
            QMessageBox::information(this, "成功", "订单取消成功！");
            showStatusMessage("订单已取消", 2000);
            on_btn_query_clicked(); // 刷新航班列表
            on_btn_refresh_orders_clicked(); // 刷新订单列表
        } else {
            QMessageBox::warning(this, "失败", "订单取消失败");
            showStatusMessage("取消失败", 2000);
        }
    }
}

// 当航班表格选择改变时自动填充航班ID
void MainWindow::on_table_flights_itemSelectionChanged()
{
    int currentRow = ui->table_flights->currentRow();
    if (currentRow >= 0) {
        QTableWidgetItem* idItem = ui->table_flights->item(currentRow, 0);
        if (idItem) {
            int flightId = idItem->text().toInt();
            // 这里需要检查是否有spinBox_flightId控件，如果没有就注释掉这行
            // ui->spinBox_flightId->setValue(flightId);

            // 显示选中航班的详细信息
            Flight flight = DatabaseManager::instance().getFlightById(flightId);
            if (flight.id != -1) {
                QString info = QString("已选中: %1 %2→%3 经济舱:￥%4 公务舱:￥%5")
                                   .arg(flight.flightNumber, flight.origin, flight.destination,
                                        QString::number(flight.economyPrice), QString::number(flight.businessPrice));
                showStatusMessage(info, 3000);
            }
        }
    }
}

void MainWindow::on_btn_clear_conditions_clicked()
{
    // 清空查询条件输入框
    ui->lineEdit_origin->clear();
    ui->lineEdit_dest->clear();

    // 重置时间范围到默认值
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime startOfDay = QDateTime(currentDateTime.date(), QTime(0, 0, 0));
    QDateTime endOfDay = QDateTime(currentDateTime.date(), QTime(23, 59, 59));

    ui->dateTimeEdit_from->setDateTime(startOfDay);
    ui->dateTimeEdit_to->setDateTime(endOfDay.addDays(30)); // 默认查询未来30天

    // 重置排序方式为默认（价格从低到高）
    ui->radioButton_price_low_high->setChecked(true);

    // 显示状态消息
    showStatusMessage("查询条件已清空", 1500);

    // 重新查询显示所有航班
    on_btn_query_clicked();
}
void MainWindow::checkFlightReminders()
{
    if (!m_isLoggedIn || !m_currentUser) return;

    QList<Order> orders = DatabaseManager::instance().getUserOrders(m_currentUser->id);
    QDateTime now = QDateTime::currentDateTime();

    for (const Order& order : std::as_const(orders)) {
        if (order.status == "cancelled" || order.status == "completed") continue;

        Flight flight = DatabaseManager::instance().getFlightById(order.flightId);
        if (flight.id == -1) continue;

        qint64 secsToDeparture = now.secsTo(flight.departureTime);
        
        // 起飞前2小时提醒
        if (secsToDeparture > 0 && secsToDeparture <= 7200) {
            showStatusMessage(QString("温馨提示：您的航班 %1 即将起飞，请尽快前往登机口 %2").arg(flight.flightNumber, order.gate), 10000);
        }
        
        // 自动更新状态：行程中
        if (now >= flight.departureTime && now <= flight.arrivalTime && order.status != "traveling") {
            QSqlQuery query;
            query.prepare("UPDATE orders SET status = 'traveling' WHERE id = ?");
            query.addBindValue(order.id);
            query.exec();
        }
        
        // 自动更新状态：已完成
        if (now > flight.arrivalTime && order.status != "completed") {
            QSqlQuery query;
            query.prepare("UPDATE orders SET status = 'completed' WHERE id = ?");
            query.addBindValue(order.id);
            query.exec();
        }
    }
}

//订单刷新功能
void MainWindow::on_btn_refresh_orders_clicked()
{
    if (!m_isLoggedIn || !m_currentUser) {
        QMessageBox::warning(this, "警告", "请先登录");
        return;
    }

    ui->list_orders->clear();
    QList<Order> orders = DatabaseManager::instance().getUserOrders(m_currentUser->id);
    QDateTime now = QDateTime::currentDateTime();

    for (const Order& order : std::as_const(orders)) {
        // 获取航班信息
        Flight flight = DatabaseManager::instance().getFlightById(order.flightId);

        QString statusText = order.status;
        if (order.status == "booked") statusText = "已出票";
        else if (order.status == "traveling") statusText = "行程中";
        else if (order.status == "completed") statusText = "已完成";
        else if (order.status == "cancelled") statusText = "已取消";

        QString itemText = QString("订单ID: %1 | 航班: %2 %3→%4 | 舱位: %5 | 座位: %6 | 登机口: %7 | 状态: %8")
                               .arg(order.id)
                               .arg(flight.flightNumber)
                               .arg(flight.origin)
                               .arg(flight.destination)
                               .arg(order.seatClass == "economy" ? "经济舱" : "公务舱")
                               .arg(order.seatNumber)
                               .arg(order.gate)
                               .arg(statusText);

        // 如果在行程中，增加进度信息
        if (order.status == "traveling") {
            qint64 totalSecs = flight.departureTime.secsTo(flight.arrivalTime);
            qint64 elapsedSecs = flight.departureTime.secsTo(now);
            int progress = (totalSecs > 0) ? (elapsedSecs * 100 / totalSecs) : 100;
            itemText += QString(" | 飞行进度: %1%").arg(progress);
        }

        QListWidgetItem* item = new QListWidgetItem(itemText);

        // 根据不同状态设置不同颜色
        if (order.status == "cancelled") {
            item->setForeground(QColor(128, 128, 128));
        } else if (order.status == "completed") {
            item->setForeground(QColor(40, 167, 69));
        } else if (order.status == "traveling") {
            item->setForeground(QColor(0, 120, 215));
            item->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        } else {
            item->setForeground(QColor(0, 123, 255));
        }

        ui->list_orders->addItem(item);
    }

    ui->lbl_order_stats->setText(QString("共有 %1 个订单").arg(orders.size()));
    showStatusMessage(QString("已加载 %1 个订单").arg(orders.size()), 2000);
}
// ==================== 辅助函数实现 ====================
void MainWindow::displayFlights(const QList<Flight>& flights)
{
    ui->table_flights->setRowCount(0);

    for (const Flight& flight : std::as_const(flights)) {
        int row = ui->table_flights->rowCount();
        ui->table_flights->insertRow(row);

        // 填充航班数据 - 使用新的字段名
        ui->table_flights->setItem(row, 0, new QTableWidgetItem(QString::number(flight.id)));
        ui->table_flights->setItem(row, 1, new QTableWidgetItem(flight.flightNumber));
        ui->table_flights->setItem(row, 2, new QTableWidgetItem(flight.origin));
        ui->table_flights->setItem(row, 3, new QTableWidgetItem(flight.destination));

        // 显示时间信息
        QString departureTimeStr = flight.departureTime.isValid() ?
                                       flight.departureTime.toString("yyyy-MM-dd hh:mm") : "未设置";
        QString arrivalTimeStr = flight.arrivalTime.isValid() ?
                                     flight.arrivalTime.toString("yyyy-MM-dd hh:mm") : "未设置";

        ui->table_flights->setItem(row, 4, new QTableWidgetItem(departureTimeStr));
        ui->table_flights->setItem(row, 5, new QTableWidgetItem(arrivalTimeStr));
        ui->table_flights->setItem(row, 6, new QTableWidgetItem(flight.airline.isEmpty() ? "未知" : flight.airline));

        // 使用新的价格字段
        ui->table_flights->setItem(row, 7, new QTableWidgetItem(QString::number(flight.economyPrice, 'f', 2)));
        ui->table_flights->setItem(row, 8, new QTableWidgetItem(QString::number(flight.businessPrice, 'f', 2)));

        // 使用新的座位字段
        ui->table_flights->setItem(row, 9, new QTableWidgetItem(QString::number(flight.availableEconomySeats)));
        ui->table_flights->setItem(row, 10, new QTableWidgetItem(QString::number(flight.availableBusinessSeats)));
        ui->table_flights->setItem(row, 11, new QTableWidgetItem(flight.aircraftModel.isEmpty() ? "未知" : flight.aircraftModel));
        ui->table_flights->setItem(row, 12, new QTableWidgetItem(flight.hasMeal ? "是" : "否"));

        // 设置对齐方式
        for (int col = 7; col <= 10; ++col) {
            QTableWidgetItem* item = ui->table_flights->item(row, col);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }

        // 余票不足时显示红色警告
        if (flight.availableEconomySeats < 5 || flight.availableBusinessSeats < 2) {
            for (int col = 0; col < 13; ++col) {
                QTableWidgetItem* item = ui->table_flights->item(row, col);
                if (item) {
                    item->setForeground(QBrush(QColor(255, 0, 0)));
                    QFont font = item->font();
                    font.setBold(true);
                    item->setFont(font);
                    item->setToolTip("余票紧张，请尽快预订！");
                }
            }
        }
    }

    // 更新航班数量显示
    if (m_resultLabel) {
        m_resultLabel->setText(QString("找到%1个航班").arg(flights.size()));
    }
}

void MainWindow::updateUserInterface()
{
    bool loggedIn = m_isLoggedIn;

    ui->tab_order->setEnabled(loggedIn);
    ui->btn_refresh_orders->setEnabled(loggedIn);
    ui->btn_logout->setEnabled(loggedIn);

    if (loggedIn && m_currentUser) {
        ui->lbl_user_status->setText(QString("状态: 已登录 - 用户: %1 (ID: %2)").arg(m_currentUser->username, QString::number(m_currentUser->id)));
    } else {
        ui->lbl_user_status->setText("状态: 未登录");
        ui->list_orders->clear();
    }
}

void MainWindow::showStatusMessage(const QString &message, int timeout)
{
    ui->statusbar->showMessage(message, timeout);
}
