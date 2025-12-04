#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置程序编码和字体
    a.setApplicationName(QString::fromUtf8("iOS设备管理器"));
    a.setApplicationDisplayName(QString::fromUtf8("iOS设备管理器 - libimobiledevice示例"));
    a.setApplicationVersion(QString::fromUtf8("1.0.0"));
    a.setOrganizationName(QString::fromUtf8("JTStudio"));
    a.setOrganizationDomain(QString::fromUtf8("jtstudio.com"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = QString::fromUtf8("phone-linkc_") + QLocale(locale).name();
        if (translator.load(QString::fromUtf8(":/i18n/") + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
