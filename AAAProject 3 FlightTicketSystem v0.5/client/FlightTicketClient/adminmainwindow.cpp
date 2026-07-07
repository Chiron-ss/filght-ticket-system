#include "adminmainwindow.h"
#include "ui_adminmainwindow.h"
#include <QtWidgets/QMessageBox>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtWidgets/QInputDialog>
#include <QtSql/QSqlError>
#include <utility>

AdminMainWindow::AdminMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminMainWindow)
    , m_isAdminLoggedIn(false)
{
    ui->setupUi(this);
    setWindowTitle("航班票务系统 - 管理员后台");
    resize(1200, 900);

    // 初始化界面
    initAdminUI();
    QDateTime currentTime = QDateTime::currentDateTime();
    ui->dateTimeEditDepart->setDateTime(currentTime);
    ui->dateTimeEditArrive->setDateTime(currentTime.addSecs(3600));
    initConnections();

    // 初始加载数据
    on_btn_refresh_flights_clicked();
    on_btn_refresh_users_clicked();
    on_btn_refresh_orders_admin_clicked();
    on_btn_refresh_stats_clicked();

    showAdminStatus("管理端初始化完成", 3000);
}

AdminMainWindow::~AdminMainWindow()
{
    delete ui;
}

// 管理员登录验证（仅超级管理员/指定角色可登录）
bool AdminMainWindow::adminLogin(const QString& username, const QString& password)
{
    // 模拟管理员验证（实际项目需在数据库添加admin_users表，此处简化）
    // 生产环境需替换为数据库查询：SELECT * FROM admin_users WHERE username=? AND password=?
    if ((username == "admin" && password == "admin123") ||
        (username == "flight_admin" && password == "flight123")) {

        m_currentAdmin.username = username;
        m_currentAdmin.role = (username == "admin") ? "super_admin" : "flight_admin";
        m_currentAdmin.isActive = true;
        m_currentAdmin.lastLogin = QDateTime::currentDateTime();
        m_isAdminLoggedIn = true;

        // 根据角色设置权限
        if (m_currentAdmin.role == "super_admin") {
            m_currentAdmin.permissions["canManageFlights"] = true;
            m_currentAdmin.permissions["canManageOrders"] = true;
            m_currentAdmin.permissions["canManageUsers"] = true;
            m_currentAdmin.permissions["canViewStatistics"] = true;
            m_currentAdmin.permissions["canSystemSettings"] = true;
        } else if (m_currentAdmin.role == "flight_admin") {
            m_currentAdmin.permissions["canManageFlights"] = true;
            m_currentAdmin.permissions["canManageOrders"] = false;
            m_currentAdmin.permissions["canManageUsers"] = false;
            m_currentAdmin.permissions["canViewStatistics"] = true;
            m_currentAdmin.permissions["canSystemSettings"] = false;
        }

        // 根据权限禁用/启用控件
        ui->tabUser->setEnabled(m_currentAdmin.permissions["canManageUsers"]);
        ui->tabOrder->setEnabled(m_currentAdmin.permissions["canManageOrders"]);
        ui->btnResetSystem->setEnabled(m_currentAdmin.permissions["canSystemSettings"]);
        ui->btnBlockUser->setEnabled(m_currentAdmin.permissions["canManageUsers"]);
        ui->btnUpdateOrder->setEnabled(m_currentAdmin.permissions["canManageOrders"]);

        ui->lblAdminStatus->setText(QString("当前登录：%1（%2） | 最后登录：%3")
                                          .arg(m_currentAdmin.username, m_currentAdmin.role, m_currentAdmin.lastLogin.toString("yyyy-MM-dd HH:mm")));

        showAdminStatus(QString("管理员 %1 登录成功").arg(username), 3000);
        return true;
    }

    showAdminStatus("管理员登录失败：用户名/密码错误", 3000);
    QMessageBox::warning(this, "登录失败", "管理员用户名或密码错误！");
    return false;
}

// 初始化管理端界面
void AdminMainWindow::initAdminUI()
{
    // 初始化航班表格（管理员版，含更多操作列）
    ui->tableFlight->setColumnCount(15);
    QStringList flightHeaders = {"ID", "航班号", "出发地", "目的地", "起飞时间", "到达时间",
                                 "航空公司", "经济舱价", "公务舱价", "经济舱总座", "经济舱余座",
                                 "公务舱总座", "公务舱余座", "机型", "餐食"};
    ui->tableFlight->setHorizontalHeaderLabels(flightHeaders);
    ui->tableFlight->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableFlight->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableFlight->setAlternatingRowColors(true);

    // 设置列宽
    ui->tableFlight->setColumnWidth(0, 50);
    ui->tableFlight->setColumnWidth(1, 100);
    ui->tableFlight->setColumnWidth(4, 150);
    ui->tableFlight->setColumnWidth(5, 150);

    // 初始化用户表格
    ui->tableUser->setColumnCount(5);
    QStringList userHeaders = {"用户ID", "用户名", "密码", "电话", "创建时间"};
    ui->tableUser->setHorizontalHeaderLabels(userHeaders);
    ui->tableUser->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableUser->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 初始化订单表格（管理员版）
    ui->tableOrder->setColumnCount(8);
    QStringList orderHeaders = {"订单ID", "用户ID", "航班ID", "舱位", "下单时间", "状态", "航班号", "用户名"};
    ui->tableOrder->setHorizontalHeaderLabels(orderHeaders);
    ui->tableOrder->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableOrder->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 初始化统计面板
    ui->lblTotalFlights->setText("0");
    ui->lblActiveFlights->setText("0");
    ui->lblTotalOrders->setText("0");
    ui->lblCompletedOrders->setText("0");
    ui->lblTotalUsers->setText("0");
    ui->lblActiveUsers->setText("0");
    ui->lblTotalRevenue->setText("¥0.00");
    ui->lblTodayRevenue->setText("¥0.00");
}

// 绑定信号槽
void AdminMainWindow::initConnections()
{
    // Qt自动连接机制已覆盖命名规范的槽函数，此处可补充自定义绑定
}

// ========== 航班管理槽函数 ==========
void AdminMainWindow::on_btn_add_flight_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageFlights"]) {
        QMessageBox::warning(this, "权限不足", "您无权限操作航班！");
        return;
    }

    // 读取UI界面输入的航班信息
    QString flightNumber = ui->lineEditFlightNum->text().trimmed();
    QString origin = ui->lineEditOrigin->text().trimmed();
    QString destination = ui->lineEditDest->text().trimmed();
    QDateTime departureTime = ui->dateTimeEditDepart->dateTime();
    QDateTime arrivalTime = ui->dateTimeEditArrive->dateTime();
    QString airline = ui->lineEditAirline->text().trimmed();
    double economyPrice = ui->doubleSpinBoxEcoPrice->value();
    double businessPrice = ui->doubleSpinBoxBusPrice->value();
    int totalEconomySeats = ui->spinBoxEcoTotal->value();
    int availableEconomySeats = ui->spinBoxEcoAvail->value();
    int totalBusinessSeats = ui->spinBoxBusTotal->value();
    int availableBusinessSeats = ui->spinBoxBusAvail->value();
    QString aircraftModel = ui->lineEditAircraft->text().trimmed();
    bool hasMeal = ui->checkBoxMeal->isChecked();

    // 输入验证
    if (flightNumber.isEmpty() || origin.isEmpty() || destination.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "航班号、出发地、目的地不能为空！");
        return;
    }

    if (departureTime >= arrivalTime) {
        QMessageBox::warning(this, "时间错误", "到达时间必须晚于出发时间！");
        return;
    }

    if (totalEconomySeats < availableEconomySeats || totalBusinessSeats < availableBusinessSeats) {
        QMessageBox::warning(this, "座位数错误", "可用座位数不能大于总座位数！");
        return;
    }

    // 构建航班对象
    Flight newFlight;
    newFlight.flightNumber = flightNumber;
    newFlight.origin = origin;
    newFlight.destination = destination;
    newFlight.departureTime = departureTime;
    newFlight.arrivalTime = arrivalTime;
    newFlight.airline = airline;
    newFlight.economyPrice = economyPrice;
    newFlight.businessPrice = businessPrice;
    newFlight.totalEconomySeats = totalEconomySeats;
    newFlight.availableEconomySeats = availableEconomySeats;
    newFlight.totalBusinessSeats = totalBusinessSeats;
    newFlight.availableBusinessSeats = availableBusinessSeats;
    newFlight.aircraftModel = aircraftModel;
    newFlight.hasMeal = hasMeal;

    bool operationSuccess = false;
    QString operationMsg;

    // 区分添加/编辑状态
    if (m_editingFlightId == -1) {
        // 新增航班逻辑（原有逻辑）
        operationSuccess = DatabaseManager::instance().addFlight(newFlight);
        operationMsg = operationSuccess ? "航班添加成功！" : "航班添加失败！";
        showAdminStatus(QString("添加航班 %1 %2").arg(flightNumber, operationSuccess ? "成功" : "失败"), 3000);
    } else {
        // 编辑航班逻辑
        newFlight.id = m_editingFlightId; // 绑定编辑的航班ID
        operationSuccess = DatabaseManager::instance().updateFlight(newFlight);
        operationMsg = operationSuccess ? "航班编辑成功！" : "航班编辑失败！";
        showAdminStatus(QString("编辑航班 %1 %2").arg(flightNumber, operationSuccess ? "成功" : "失败"), 3000);

        // 编辑成功后重置状态
        if (operationSuccess) {
            m_editingFlightId = -1;
            ui->btn_add_flight->setText("添加航班");
        }
    }

    // 处理操作结果
    if (operationSuccess) {
        QMessageBox::information(this, "成功", operationMsg);
        // 清空输入框
        ui->lineEditFlightNum->clear();
        ui->lineEditOrigin->clear();
        ui->lineEditDest->clear();
        ui->lineEditAirline->clear();
        ui->lineEditAircraft->clear();
        ui->doubleSpinBoxEcoPrice->setValue(0.01);
        ui->doubleSpinBoxBusPrice->setValue(0.01);
        ui->spinBoxEcoTotal->setValue(1);
        ui->spinBoxEcoAvail->setValue(0);
        ui->spinBoxBusTotal->setValue(1);
        ui->spinBoxBusAvail->setValue(0);
        ui->checkBoxMeal->setChecked(false);

        QDateTime now = QDateTime::currentDateTime();
        ui->dateTimeEditDepart->setDateTime(now);
        ui->dateTimeEditArrive->setDateTime(now.addSecs(3600));

        // 刷新航班列表
        QList<Flight> allFlights = DatabaseManager::instance().getAllFlights();
        std::sort(allFlights.begin(), allFlights.end(),
                  [](const Flight& a, const Flight& b) {
                      return a.departureTime < b.departureTime;
                  });
        displayFlightsAdmin(allFlights);
    } else {
        QMessageBox::warning(this, "失败", operationMsg);
    }
}
void AdminMainWindow::on_btn_edit_flight_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageFlights"]) {
        QMessageBox::warning(this, "权限不足", "您无权限编辑航班！");
        return;
    }

    int currentRow = ui->tableFlight->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选中要编辑的航班！");
        return;
    }

    // 3. 声明局部变量（核心修复：之前遗漏了这两行的变量声明）
    int flightId = ui->tableFlight->item(currentRow, 0)->text().toInt(); // 声明flightId
    Flight flight = DatabaseManager::instance().getFlightById(flightId); // 声明flight

    // 4. 校验航班是否存在
    if (flight.id == -1) {
        QMessageBox::warning(this, "错误", "航班不存在！");
        return;
    }

    // 标记当前编辑状态
    m_editingFlightId = flightId;

    // 将航班数据填充到原有添加航班的输入框
    ui->lineEditFlightNum->setText(flight.flightNumber);
    ui->lineEditOrigin->setText(flight.origin);
    ui->lineEditDest->setText(flight.destination);
    ui->dateTimeEditDepart->setDateTime(flight.departureTime);
    ui->dateTimeEditArrive->setDateTime(flight.arrivalTime);
    ui->lineEditAirline->setText(flight.airline);
    ui->doubleSpinBoxEcoPrice->setValue(flight.economyPrice);
    ui->doubleSpinBoxBusPrice->setValue(flight.businessPrice);
    ui->spinBoxEcoTotal->setValue(flight.totalEconomySeats);
    ui->spinBoxEcoAvail->setValue(flight.availableEconomySeats);
    ui->spinBoxBusTotal->setValue(flight.totalBusinessSeats);
    ui->spinBoxBusAvail->setValue(flight.availableBusinessSeats);
    ui->lineEditAircraft->setText(flight.aircraftModel);
    ui->checkBoxMeal->setChecked(flight.hasMeal);

    // 修改按钮文本为“保存编辑”
    ui->btn_add_flight->setText("保存编辑");
    showAdminStatus(QString("已加载航班 %1 信息，可修改后点击「保存编辑」").arg(flight.flightNumber), 3000);
}

void AdminMainWindow::on_btn_delete_flight_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageFlights"]) {
        QMessageBox::warning(this, "权限不足", "您无权限删除航班！");
        return;
    }

    int currentRow = ui->tableFlight->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选中要删除的航班！");
        return;
    }

    if (!confirmOperation("删除确认", "确定要删除选中的航班吗？此操作不可恢复！")) {
        return;
    }

    int flightId = ui->tableFlight->item(currentRow, 0)->text().toInt();
    // 注意：DatabaseManager 需新增 deleteFlight 接口（见下文扩展）
    QSqlQuery query;
    query.prepare("DELETE FROM flights WHERE id = ?");
    query.addBindValue(flightId);

    if (query.exec()) {
        QMessageBox::information(this, "成功", "航班删除成功！");
        on_btn_refresh_flights_clicked();
        showAdminStatus(QString("删除航班ID %1 成功").arg(flightId), 3000);
    } else {
        QMessageBox::warning(this, "失败", "航班删除失败（可能关联订单）！");
        showAdminStatus(QString("删除航班ID %1 失败：%2").arg(QString::number(flightId), query.lastError().text()), 3000);
    }
}

void AdminMainWindow::on_btn_refresh_flights_clicked()
{
    // 重置编辑状态
    if (m_editingFlightId != -1) {
        m_editingFlightId = -1;
        ui->btn_add_flight->setText("添加航班");
    }
    QList<Flight> allFlights = DatabaseManager::instance().getAllFlights();
    displayFlightsAdmin(allFlights);
    showAdminStatus(QString("刷新航班列表，共 %1 个航班").arg(allFlights.size()), 2000);
}

void AdminMainWindow::on_btn_query_flights_admin_clicked()
{
    QString origin = ui->lineEditFlightOrigin->text().trimmed();
    QString destination = ui->lineEditFlightDest->text().trimmed();
    QList<Flight> filteredFlights = DatabaseManager::instance().queryFlights(origin, destination, QDate());
    displayFlightsAdmin(filteredFlights);
    showAdminStatus(QString("查询到 %1 个航班（%2→%3）").arg(QString::number(filteredFlights.size()), origin, destination), 2000);
}

// ========== 用户管理槽函数 ==========
void AdminMainWindow::on_btn_refresh_users_clicked()
{
    // 扩展DatabaseManager：查询所有用户
    QList<User> allUsers;
    QSqlQuery query("SELECT id, username, password, phone, create_time FROM users ORDER BY id DESC");
    while (query.next()) {
        User user;
        user.id = query.value("id").toInt();
        user.username = query.value("username").toString();
        user.password = query.value("password").toString(); // 生产环境需加密
        user.phone = query.value("phone").toString();
        user.createTime = query.value("create_time").toDateTime();
        allUsers.append(user);
    }

    displayUsers(allUsers);
    showAdminStatus(QString("刷新用户列表，共 %1 个用户").arg(allUsers.size()), 2000);
}

void AdminMainWindow::on_btnBlockUser_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageUsers"]) {
        QMessageBox::warning(this, "权限不足", "您无权限管理用户！");
        return;
    }

    int currentRow = ui->tableUser->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选中要操作的用户！");
        return;
    }

    QString username = ui->tableUser->item(currentRow, 1)->text();

    if (confirmOperation("用户操作", QString("确定要禁用/启用用户 %1 吗？").arg(username))) {
        // 扩展DatabaseManager：添加用户状态字段（生产环境需修改users表）
        QMessageBox::information(this, "提示", "用户禁用功能需扩展users表添加is_blocked字段，此处仅演示！");
        on_btn_refresh_users_clicked();
    }
}

// ========== 用户查询功能实现 ==========
void AdminMainWindow::on_btnQueryUser_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageUsers"]) {
        QMessageBox::warning(this, "权限不足", "您无权限查询用户！");
        return;
    }

    QString username = ui->lineEditUserQuery->text().trimmed();
    QList<User> filteredUsers;

    if (username.isEmpty()) {
        // 如果输入为空，显示所有用户
        on_btn_refresh_users_clicked();
        return;
    }

    // 根据用户名模糊查询
    QSqlQuery query;
    query.prepare("SELECT id, username, password, phone, create_time FROM users WHERE username LIKE ? ORDER BY id DESC");
    query.addBindValue("%" + username + "%");  // 模糊匹配

    if (query.exec()) {
        while (query.next()) {
            User user;
            user.id = query.value("id").toInt();
            user.username = query.value("username").toString();
            user.password = query.value("password").toString();
            user.phone = query.value("phone").toString();
            user.createTime = query.value("create_time").toDateTime();
            filteredUsers.append(user);
        }
    } else {
        qDebug() << "用户查询失败:" << query.lastError().text();
        QMessageBox::warning(this, "查询失败", "用户查询出错：" + query.lastError().text());
        return;
    }

    // 显示查询结果
    displayUsers(filteredUsers);
    showAdminStatus(QString("查询到 %1 个匹配的用户（用户名包含：%2）").arg(filteredUsers.size()).arg(username), 3000);
}

void AdminMainWindow::on_btnClearUserQuery_clicked()
{
    // 清空用户名输入框
    ui->lineEditUserQuery->clear();
    // 可选：清空后自动刷新全部用户（保持界面一致性）
    on_btn_refresh_users_clicked();
    // 状态提示
    ui->lblAdminStatus->setText("用户查询条件已清空");
}

// ========== 新增：清空航班查询条件 ==========
void AdminMainWindow::on_btnClearFlightQuery_clicked()
{
    // 清空出发地、目的地输入框
    ui->lineEditFlightOrigin->clear();
    ui->lineEditFlightDest->clear();
    // 重置日期选择框为当前日期
    ui->dateEditFlight->setDate(QDate::currentDate());
    // 可选：清空后自动刷新全部航班
    on_btn_refresh_flights_clicked();
    // 状态提示
    ui->lblAdminStatus->setText("航班查询条件已清空");
}
// ========== 订单管理槽函数 ==========
// ========== 订单查询功能实现 ==========
void AdminMainWindow::on_btnQueryOrder_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageOrders"]) {
        QMessageBox::warning(this, "权限不足", "您无权限查询订单！");
        return;
    }

    int userId = ui->spinBoxOrderUserId->value();
    QList<Order> userOrders;

    // 根据用户ID查询订单
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
            order.orderTime = query.value("order_time").toDateTime();
            order.status = query.value("status").toString();
            userOrders.append(order);
        }
    } else {
        qDebug() << "订单查询失败:" << query.lastError().text();
        QMessageBox::warning(this, "查询失败", "订单查询出错：" + query.lastError().text());
        return;
    }

    // 显示查询结果
    displayOrdersAdmin(userOrders);
    showAdminStatus(QString("查询到用户ID %1 的 %2 个订单").arg(userId).arg(userOrders.size()), 3000);
}

void AdminMainWindow::on_btn_refresh_orders_admin_clicked()
{
    QList<Order> allOrders;
    QSqlQuery query("SELECT * FROM orders ORDER BY order_time DESC");
    while (query.next()) {
        Order order;
        order.id = query.value("id").toInt();
        order.userId = query.value("user_id").toInt();
        order.flightId = query.value("flight_id").toInt();
        order.seatClass = query.value("seat_class").toString();
        order.orderTime = query.value("order_time").toDateTime();
        order.status = query.value("status").toString();
        allOrders.append(order);
    }

    displayOrdersAdmin(allOrders);
    showAdminStatus(QString("刷新订单列表，共 %1 个订单").arg(allOrders.size()), 2000);
}

void AdminMainWindow::on_btnClearOrderQuery_clicked()
{
    // 重置用户ID输入框为最小值（1）
    ui->spinBoxOrderUserId->setValue(1);
    // 清空后自动刷新全部订单
    on_btn_refresh_orders_admin_clicked();
    // 状态提示
    ui->lblAdminStatus->setText("订单查询条件已清空");
}

void AdminMainWindow::on_btnUpdateOrder_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canManageOrders"]) {
        QMessageBox::warning(this, "权限不足", "您无权限修改订单状态！");
        return;
    }

    // 1. 获取订单ID（优先输入框，其次表格选中行）
    int orderId = ui->spinBoxOrderId->value();
    bool isIdValid = (orderId > 0);

    if (!isIdValid) {
        int currentRow = ui->tableOrder->currentRow();
        if (currentRow < 0) {
            QMessageBox::warning(this, "提示", "请输入有效订单ID，或选中表格中的订单行！");
            return;
        }
        orderId = ui->tableOrder->item(currentRow, 0)->text().toInt();
        isIdValid = (orderId > 0);
    }

    if (!isIdValid) {
        QMessageBox::warning(this, "错误", "订单ID无效，请输入正确的订单ID！");
        return;
    }

    // 2. 校验订单是否存在
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT status FROM orders WHERE id = ?");
    checkQuery.addBindValue(orderId);
    if (!checkQuery.exec() || !checkQuery.next()) {
        QMessageBox::warning(this, "错误", QString("订单ID %1 不存在！").arg(orderId));
        return;
    }
    QString oldStatus = checkQuery.value(0).toString(); // 获取订单当前状态

    // 3. 获取 combo 选中的新状态
    QString newStatus = ui->comboOrderStatus->currentText().trimmed();
    if (newStatus.isEmpty()) {
        QMessageBox::warning(this, "错误", "请选择新的订单状态！");
        return;
    }

    // 4. 状态未变化时提示
    if (newStatus == oldStatus) {
        QMessageBox::information(this, "提示", QString("订单ID %1 当前状态已是“%2”，无需修改！").arg(orderId).arg(newStatus));
        return;
    }

    // 5. 弹出确认对话框
    QMessageBox::StandardButton ret = QMessageBox::question(
        this, "确认更新",
        QString("确定将订单ID %1 的状态从“%2”更新为“%3”吗？").arg(orderId).arg(oldStatus).arg(newStatus),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
        );

    if (ret != QMessageBox::Yes) {
        showAdminStatus("已取消订单状态更新", 2000);
        return;
    }

    // 6. 执行更新
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE orders SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(orderId);

    if (updateQuery.exec()) {
        QMessageBox::information(this, "成功", QString("订单ID %1 状态已更新为“%2”！").arg(orderId).arg(newStatus));
        on_btn_refresh_orders_admin_clicked(); // 刷新订单表格
        // 重置输入框（可选）
        if (ui->spinBoxOrderId->value() == orderId) {
            ui->spinBoxOrderId->setValue(0);
        }
        showAdminStatus(QString("修改订单ID %1 状态为 %2").arg(orderId).arg(newStatus), 3000);
    } else {
        QMessageBox::warning(this, "失败", "订单状态修改失败！\n原因：" + updateQuery.lastError().text());
    }
}
void AdminMainWindow::on_tableOrder_currentCellChanged(int currentRow, int currentCol, int previousRow, int previousCol)
{
    Q_UNUSED(currentCol);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousCol);

    if (currentRow >= 0) {
        // 1. 填充订单ID到 spinBoxOrderId
        int orderId = ui->tableOrder->item(currentRow, 0)->text().toInt();
        ui->spinBoxOrderId->setValue(orderId);

        // 2. 同步当前订单状态到 comboOrderStatus
        QString currentStatus = ui->tableOrder->item(currentRow, 5)->text(); // 表格第5列是“状态”列
        // 匹配 combo 的选项（注意：表格中状态是中文/英文需对应，这里假设表格中是英文状态）
        int statusIndex = ui->comboOrderStatus->findText(currentStatus);
        if (statusIndex != -1) {
            ui->comboOrderStatus->setCurrentIndex(statusIndex);
        } else {
            ui->comboOrderStatus->setCurrentIndex(0); // 匹配失败时默认选第一个
        }
    } else {
        // 未选中行时重置
        ui->spinBoxOrderId->setValue(0);
        ui->comboOrderStatus->setCurrentIndex(0);
    }
}
// ========== 统计与设置槽函数 ==========
void AdminMainWindow::on_btn_refresh_stats_clicked()
{
    loadSystemStats();
    updateStatsDisplay();
    showAdminStatus("刷新系统统计数据完成", 2000);
}

void AdminMainWindow::on_btnResetSystem_clicked()
{
    if (!m_isAdminLoggedIn || !m_currentAdmin.permissions["canSystemSettings"]) {
        QMessageBox::warning(this, "权限不足", "您无权限重置系统！");
        return;
    }

    if (confirmOperation("危险操作", "确定要重置所有系统数据吗？此操作不可恢复！")) {
        if (DatabaseManager::instance().clearTestData()) {
            QMessageBox::information(this, "成功", "系统数据已重置！");
            on_btn_refresh_flights_clicked();
            on_btn_refresh_users_clicked();
            on_btn_refresh_orders_admin_clicked();
            on_btn_refresh_stats_clicked();
            showAdminStatus("系统数据重置完成", 3000);
        } else {
            QMessageBox::warning(this, "失败", "系统数据重置失败！");
        }
    }
}

// ========== 界面交互槽函数 ==========
void AdminMainWindow::on_tableFlight_itemSelectionChanged()
{
    int currentRow = ui->tableFlight->currentRow();
    if (currentRow >= 0) {
        QString flightNumber = ui->tableFlight->item(currentRow, 1)->text();
        showAdminStatus(QString("已选中航班：%1").arg(flightNumber), 2000);
    }
}

void AdminMainWindow::on_btn_logout_admin_clicked()
{
    m_isAdminLoggedIn = false;
    m_currentAdmin = AdminUser(); // 清空管理员信息
    ui->lblAdminStatus->setText("当前登录：未登录");
    ui->tabUser->setEnabled(false);
    ui->tabOrder->setEnabled(false);
    ui->btnResetSystem->setEnabled(false);
    QMessageBox::information(this, "退出成功", "管理员已退出登录！");
    showAdminStatus("管理员退出登录", 3000);
    this->close(); // 关闭管理端窗口
}

// ========== 数据展示辅助函数 ==========
void AdminMainWindow::displayFlightsAdmin(const QList<Flight>& flights)
{
    ui->tableFlight->setRowCount(0);
    for (const Flight& flight : std::as_const(flights)) {
        int row = ui->tableFlight->rowCount();
        ui->tableFlight->insertRow(row);

        ui->tableFlight->setItem(row, 0, new QTableWidgetItem(QString::number(flight.id)));
        ui->tableFlight->setItem(row, 1, new QTableWidgetItem(flight.flightNumber));
        ui->tableFlight->setItem(row, 2, new QTableWidgetItem(flight.origin));
        ui->tableFlight->setItem(row, 3, new QTableWidgetItem(flight.destination));
        ui->tableFlight->setItem(row, 4, new QTableWidgetItem(flight.departureTime.toString("yyyy-MM-dd HH:mm")));
        ui->tableFlight->setItem(row, 5, new QTableWidgetItem(flight.arrivalTime.toString("yyyy-MM-dd HH:mm")));
        ui->tableFlight->setItem(row, 6, new QTableWidgetItem(flight.airline));
        ui->tableFlight->setItem(row, 7, new QTableWidgetItem(QString::number(flight.economyPrice, 'f', 2)));
        ui->tableFlight->setItem(row, 8, new QTableWidgetItem(QString::number(flight.businessPrice, 'f', 2)));
        ui->tableFlight->setItem(row, 9, new QTableWidgetItem(QString::number(flight.totalEconomySeats)));
        ui->tableFlight->setItem(row, 10, new QTableWidgetItem(QString::number(flight.availableEconomySeats)));
        ui->tableFlight->setItem(row, 11, new QTableWidgetItem(QString::number(flight.totalBusinessSeats)));
        ui->tableFlight->setItem(row, 12, new QTableWidgetItem(QString::number(flight.availableBusinessSeats)));
        ui->tableFlight->setItem(row, 13, new QTableWidgetItem(flight.aircraftModel));
        ui->tableFlight->setItem(row, 14, new QTableWidgetItem(flight.hasMeal ? "是" : "否"));

        // 余票不足标红
        if (flight.availableEconomySeats < 5 || flight.availableBusinessSeats < 2) {
            for (int col = 0; col <= 14; ++col) {
                QTableWidgetItem* item = ui->tableFlight->item(row, col);
                if (item) {
                    item->setForeground(QColor(255, 0, 0));
                    item->setFont(QFont("Arial", 10, QFont::Bold));
                }
            }
        }
    }
}

void AdminMainWindow::displayUsers(const QList<User>& users)
{
    ui->tableUser->setRowCount(0);
    for (const User& user : std::as_const(users)) {
        int row = ui->tableUser->rowCount();
        ui->tableUser->insertRow(row);

        ui->tableUser->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        ui->tableUser->setItem(row, 1, new QTableWidgetItem(user.username));
        ui->tableUser->setItem(row, 2, new QTableWidgetItem("******")); // 隐藏密码
        ui->tableUser->setItem(row, 3, new QTableWidgetItem(user.phone));
        ui->tableUser->setItem(row, 4, new QTableWidgetItem(user.createTime.toString("yyyy-MM-dd HH:mm")));
    }
}

void AdminMainWindow::displayOrdersAdmin(const QList<Order>& orders)
{
    ui->tableOrder->setRowCount(0);
    for (const Order& order : std::as_const(orders)) {
        int row = ui->tableOrder->rowCount();
        ui->tableOrder->insertRow(row);

        // 基础订单信息
        ui->tableOrder->setItem(row, 0, new QTableWidgetItem(QString::number(order.id)));
        ui->tableOrder->setItem(row, 1, new QTableWidgetItem(QString::number(order.userId)));
        ui->tableOrder->setItem(row, 2, new QTableWidgetItem(QString::number(order.flightId)));
        ui->tableOrder->setItem(row, 3, new QTableWidgetItem(order.seatClass == "economy" ? "经济舱" : "公务舱"));
        ui->tableOrder->setItem(row, 4, new QTableWidgetItem(order.orderTime.toString("yyyy-MM-dd HH:mm")));
        ui->tableOrder->setItem(row, 5, new QTableWidgetItem(order.status));

        // 关联航班号
        Flight flight = DatabaseManager::instance().getFlightById(order.flightId);
        ui->tableOrder->setItem(row, 6, new QTableWidgetItem(flight.flightNumber));

        // 关联用户名
        User user = DatabaseManager::instance().getUserById(order.userId);
        ui->tableOrder->setItem(row, 7, new QTableWidgetItem(user.username));

        // 状态颜色
        QTableWidgetItem* statusItem = ui->tableOrder->item(row, 5);
        if (order.status == "cancelled") {
            statusItem->setForeground(QColor(128, 128, 128));
        } else if (order.status == "completed") {
            statusItem->setForeground(QColor(0, 128, 0));
        } else {
            statusItem->setForeground(QColor(0, 0, 255));
        }
    }
}

// ========== 统计辅助函数 ==========
void AdminMainWindow::loadSystemStats()
{
    m_systemStats.totalFlights = DatabaseManager::instance().getFlightCount();
    m_systemStats.activeFlights = m_systemStats.totalFlights; // 简化：所有航班均为活跃
    m_systemStats.totalOrders = DatabaseManager::instance().getOrderCount();
    m_systemStats.totalUsers = DatabaseManager::instance().getUserCount();
    m_systemStats.activeUsers = m_systemStats.totalUsers;     // 简化：所有用户均为活跃

    // 统计已完成订单
    QSqlQuery completedQuery("SELECT COUNT(*) FROM orders WHERE status = 'completed'");
    if (completedQuery.exec() && completedQuery.next()) {
        m_systemStats.completedOrders = completedQuery.value(0).toInt();
    }

    // 统计总收入（简化：经济舱/公务舱均价 × 订单数）
    QSqlQuery revenueQuery("SELECT SUM(CASE WHEN seat_class='economy' THEN f.economy_price ELSE f.business_price END) "
                           "FROM orders o JOIN flights f ON o.flight_id = f.id WHERE o.status != 'cancelled'");
    if (revenueQuery.exec() && revenueQuery.next()) {
        m_systemStats.totalRevenue = revenueQuery.value(0).toDouble();
    }

    // 今日收入
    QSqlQuery todayRevenueQuery("SELECT SUM(CASE WHEN seat_class='economy' THEN f.economy_price ELSE f.business_price END) "
                                "FROM orders o JOIN flights f ON o.flight_id = f.id "
                                "WHERE DATE(o.order_time) = DATE('now') AND o.status != 'cancelled'");
    if (todayRevenueQuery.exec() && todayRevenueQuery.next()) {
        m_systemStats.todayRevenue = todayRevenueQuery.value(0).toDouble();
    }
}

void AdminMainWindow::updateStatsDisplay()
{
    ui->lblTotalFlights->setText(QString::number(m_systemStats.totalFlights));
    ui->lblActiveFlights->setText(QString::number(m_systemStats.activeFlights));
    ui->lblTotalOrders->setText(QString::number(m_systemStats.totalOrders));
    ui->lblCompletedOrders->setText(QString::number(m_systemStats.completedOrders));
    ui->lblTotalUsers->setText(QString::number(m_systemStats.totalUsers));
    ui->lblActiveUsers->setText(QString::number(m_systemStats.activeUsers));
    ui->lblTotalRevenue->setText(QString("¥%1").arg(m_systemStats.totalRevenue, 0, 'f', 2));
    ui->lblTodayRevenue->setText(QString("¥%1").arg(m_systemStats.todayRevenue, 0, 'f', 2));
}

// ========== 辅助工具函数 ==========
void AdminMainWindow::showAdminStatus(const QString& msg, int timeout)
{
    ui->statusbar->showMessage(msg, timeout);
}

bool AdminMainWindow::confirmOperation(const QString& title, const QString& msg)
{
    int ret = QMessageBox::warning(this, title, msg,
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return (ret == QMessageBox::Yes);
}
