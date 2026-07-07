#ifndef ADMINTYPES_H
#define ADMINTYPES_H

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVariant>

// 前向声明所有管理端相关类
class AdminMainWindow;
class AdminService;
class FlightManagementWidget;
class OrderManagementWidget;
class UserManagementWidget;
class StatisticsWidget;
class SystemSettingsWidget;
class DashboardWidget;

// 管理端数据结构定义
struct AdminUser {
    int id;
    QString username;
    QString password;
    QString role; // 角色：super_admin, flight_admin, order_admin, user_admin
    QString email;
    QString phone;
    QDateTime createTime;
    QDateTime lastLogin;
    bool isActive;
    QMap<QString, bool> permissions;

    AdminUser() : id(-1), isActive(true) {}
};

struct SystemStats {
    int totalFlights;
    int activeFlights;
    int totalOrders;
    int completedOrders;
    int totalUsers;
    int activeUsers;
    double totalRevenue;
    double todayRevenue;

    SystemStats() : totalFlights(0), activeFlights(0), totalOrders(0),
        completedOrders(0), totalUsers(0), activeUsers(0),
        totalRevenue(0.0), todayRevenue(0.0) {}
};

struct AdminPermissions {
    bool canManageFlights;
    bool canManageOrders;
    bool canManageUsers;
    bool canViewStatistics;
    bool canSystemSettings;

    AdminPermissions() : canManageFlights(false), canManageOrders(false),
        canManageUsers(false), canViewStatistics(false),
        canSystemSettings(false) {}
};

#endif // ADMINTYPES_H
