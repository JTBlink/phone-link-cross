#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QStatusBar>

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
    void onDeviceSelectionChanged();
    void onConnectButtonClicked();
    void onRefreshButtonClicked();
    void onGetInfoButtonClicked();

private:
    void setupUI();
    void updateDeviceInfo(const QString &udid);
    void updateConnectionStatus();
    void updateInitialDisplayText();

    Ui::MainWindow *ui;
    
    // UI 组件
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QListWidget *m_deviceList;
    QTextEdit *m_infoDisplay;
    QPushButton *m_connectButton;
    QPushButton *m_refreshButton;
    QPushButton *m_getInfoButton;
    QLabel *m_statusLabel;
    
    // 业务逻辑
    DeviceManager *m_deviceManager;
    DeviceInfoManager *m_infoManager;
};

#endif // MAINWINDOW_H