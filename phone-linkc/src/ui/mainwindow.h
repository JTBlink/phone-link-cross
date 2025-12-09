#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

#include "core/device/devicemanager.h"
#include "core/device/deviceinfo.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDeviceFound(const QString &udid, const QString &name);
    void onDeviceLost(const QString &udid);
    void onDeviceConnected(const QString &udid);
    void onDeviceDisconnected();
    void onDeviceError(const QString &error);
    void onNoDevicesFound();
    void onDeviceSelectionChanged();
    void onConnectButtonClicked();
    void onRefreshButtonClicked();

private:
    void setupUI();
    void updateDeviceInfo(const QString &udid);
    void updateConnectionStatus();
    
    /**
     * @brief 统一更新显示文本的函数
     * @param infoText 信息显示区域的文本（空字符串表示不更新）
     * @param statusText 状态栏标签的文本（空字符串表示不更新）
     * @param createStatusLabel 是否需要创建状态标签（仅在初始化时使用，已废弃）
     */
    void updateDisplayText(const QString &infoText = QString(),
                          const QString &statusText = QString(),
                          bool createStatusLabel = false);
    
    // 辅助函数：获取各种场景下的显示文本
    QString getNoDeviceInfoText() const;
    QString getLibraryNotInstalledInfoText() const;
    QString getInitialInfoText() const;
    QString getInitialStatusText() const;

    Ui::MainWindow *ui;
    
    // 业务逻辑
    DeviceManager *m_deviceManager;
    DeviceInfoManager *m_infoManager;
};

#endif // MAINWINDOW_H