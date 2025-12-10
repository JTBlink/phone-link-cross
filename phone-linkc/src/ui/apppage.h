/**
 * @file apppage.h
 * @brief 应用管理页面头文件
 */

#ifndef APPPAGE_H
#define APPPAGE_H

#include <QWidget>
#include <QTreeWidget>
#include "../core/app/appmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AppPage;
}
QT_END_NAMESPACE

class AppPage : public QWidget
{
    Q_OBJECT

public:
    explicit AppPage(QWidget *parent = nullptr);
    ~AppPage();

    void setAppManager(AppManager *manager);
    void setCurrentDevice(const QString &udid);
    void clearDevice();

public slots:
    void refresh();

private slots:
    void onInstallClicked();
    void onUninstallClicked();
    void onRefreshClicked();
    void onErrorOccurred(const QString &error);
    void onAppsLoaded(const QVector<AppInfo> &apps);
    void onAppSizeUpdated(const QString &bundleId, const QString &appSize, const QString &docSize);

private:
    void setupUI();
    void loadApps();

    Ui::AppPage *ui;
    AppManager *m_appManager;
    QString m_currentUdid;
};

#endif // APPPAGE_H