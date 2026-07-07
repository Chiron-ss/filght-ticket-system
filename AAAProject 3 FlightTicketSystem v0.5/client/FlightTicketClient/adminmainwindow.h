#ifndef ADMINMAINWINDOW_H
#define ADMINMAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QLabel>
#include <QtCore/QString>
#include <QtCore/QList>
#include "AdminTypes.h"
#include "DatabaseManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AdminMainWindow; }
QT_END_NAMESPACE

class AdminMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminMainWindow(QWidget *parent = nullptr);
    ~AdminMainWindow();

    // 管理员登录验证（外部调用，如MainWindow）
    bool adminLogin(const QString& username, const QString& password);

private slots:
    // 航班管理槽函数
    void on_btn_add_flight_clicked();       // 添加航班
    void on_btn_edit_flight_clicked();      // 编辑航班
    void on_btn_delete_flight_clicked();    // 删除航班
    void on_btn_refresh_flights_clicked();  // 刷新航班列表
    void on_btn_query_flights_admin_clicked(); // 管理员查询航班
    // 清空航班查询条件
    void on_btnClearFlightQuery_clicked();

    // 用户管理槽函数
    void on_btn_refresh_users_clicked();    // 刷新用户列表
    void on_btnBlockUser_clicked();
    void on_btnQueryUser_clicked();  // 用户查询按钮    // 禁用/启用用户
    // 清空用户查询条件
    void on_btnClearUserQuery_clicked();


    // 订单管理槽函数
    void on_btn_refresh_orders_admin_clicked(); // 刷新所有订单
    void on_btnUpdateOrder_clicked();  // 更新订单状态
    void on_tableOrder_currentCellChanged(int currentRow, int currentCol, int previousRow, int previousCol);
    // 在private slots部分添加
    void on_btnQueryOrder_clicked();    // 订单查询按钮
    void on_btnClearOrderQuery_clicked();


    // 统计与设置槽函数
    void on_btn_refresh_stats_clicked();    // 刷新系统统计
    void on_btnResetSystem_clicked();     // 系统数据重置（谨慎使用）

    // 界面交互槽函数
    void on_tableFlight_itemSelectionChanged(); // 选中航班行
    void on_btn_logout_admin_clicked();     // 管理员退出登录

private:
    Ui::AdminMainWindow *ui;
    AdminUser m_currentAdmin;               // 当前登录管理员
    SystemStats m_systemStats;              // 系统统计数据
    bool m_isAdminLoggedIn;                 // 管理员登录状态
    int m_editingFlightId = -1;             // 新增：标记当前编辑的航班ID（-1=非编辑状态）

    // 初始化函数
    void initAdminUI();                     // 初始化管理端界面
    void initConnections();                 // 绑定信号槽
    void loadSystemStats();                 // 加载系统统计数据

    // 数据展示函数
    void displayFlightsAdmin(const QList<Flight>& flights); // 展示航班（管理员版）
    void displayUsers(const QList<User>& users);            // 展示所有用户
    void displayOrdersAdmin(const QList<Order>& orders);    // 展示所有订单
    void updateStatsDisplay();              // 更新统计面板

    // 辅助函数
    void showAdminStatus(const QString& msg, int timeout = 3000); // 状态栏提示
    bool confirmOperation(const QString& title, const QString& msg); // 操作确认弹窗
};

#endif // ADMINMAINWINDOW_H
