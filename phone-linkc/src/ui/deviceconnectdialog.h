#ifndef DEVICECONNECTDIALOG_H
#define DEVICECONNECTDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

class DeviceManager;

QT_BEGIN_NAMESPACE
namespace Ui {
class DeviceConnectDialog;
}
QT_END_NAMESPACE

/**
 * @brief 设备连接对话框
 * 
 * 提供设备选择和连接功能的模态对话框
 */
class DeviceConnectDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param deviceManager 设备管理器指针
     * @param parent 父窗口
     */
    explicit DeviceConnectDialog(DeviceManager *deviceManager, QWidget *parent = nullptr);
    ~DeviceConnectDialog();

    /**
     * @brief 获取选中的设备UDID
     * @return 选中设备的UDID，如果没有选中则返回空字符串
     */
    QString getSelectedDeviceUdid() const;

    /**
     * @brief 获取选中的设备名称
     * @return 选中设备的名称
     */
    QString getSelectedDeviceName() const;

private slots:
    void onDeviceFound(const QString &udid, const QString &name);
    void onDeviceLost(const QString &udid);
    void onDeviceConnected(const QString &udid);
    void onDeviceDisconnected();
    void onDeviceError(const QString &error);
    void onNoDevicesFound();
    
    void onRefreshClicked();
    void onDeviceSelectionChanged();
    void onConnectClicked();

private:
    void setupConnections();
    void refreshDeviceList();
    void updateStatus(const QString &message, bool showProgress = false);
    void updateButtonState();

    Ui::DeviceConnectDialog *ui;
    DeviceManager *m_deviceManager;
    QString m_selectedUdid;
    QString m_selectedName;
};

#endif // DEVICECONNECTDIALOG_H