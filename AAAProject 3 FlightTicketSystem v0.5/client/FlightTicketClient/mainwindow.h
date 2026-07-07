#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <QtCore/QString>
#include <QtCore/QList>

// 前向声明
class QRadioButton;

namespace Ui {
class MainWindow;
}

// 前向声明结构体
struct User;
struct Flight;
struct Order;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 用户管理槽函数
    void on_btn_register_clicked();
    void on_btn_login_clicked();
    void on_btn_logout_clicked();
    void on_btn_clear_input_clicked();
    void on_btn_admin_clicked(); // 添加管理员入口槽函数

    // 航班查询槽函数
    void on_btn_query_clicked();
    void on_btn_clear_conditions_clicked();
    void on_btn_add_test_flight_clicked();

    // 排序槽函数
    void on_radioButton_price_low_high_clicked();
    void on_radioButton_price_high_low_clicked();
    void on_radioButton_by_seats_clicked();

    // 订单管理槽函数

    void on_btn_refresh_orders_clicked();
    void on_btn_cancel_order_clicked();  // 添加取消订单槽函数
    void on_btn_quick_book_clicked();   // 添加快速预订槽函数

    // 添加缺失的槽函数声明
    void on_table_flights_itemSelectionChanged();
    void checkFlightReminders();

private:
    Ui::MainWindow *ui;
    User* m_currentUser;
    bool m_isLoggedIn;
    QLabel *m_resultLabel;
    QTimer *m_reminderTimer;

    void initFlightTable();
    void initConnections();
    void updateUserInterface();
    void showStatusMessage(const QString &message, int timeout = 0);

    // 排序和显示辅助函数
    void sortFlightsByPrice(bool ascending = true);
    void sortFlightsBySeats();
    void displayFlights(const QList<Flight>& flights);
};

#endif // MAINWINDOW_H
