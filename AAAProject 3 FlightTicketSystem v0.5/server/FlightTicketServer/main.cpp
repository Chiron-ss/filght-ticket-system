#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include "DatabaseManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "FlightTicketServer_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // 初始化数据库
    QString dbPath = QCoreApplication::applicationDirPath() + "/flight_system.db";
    if (DatabaseManager::instance().connect(dbPath)) {
        qDebug() << "数据库初始化成功，路径：" << dbPath;
    } else {
        qCritical() << "数据库初始化失败，路径：" << dbPath;
        return -1;
    }

    return a.exec();
}
