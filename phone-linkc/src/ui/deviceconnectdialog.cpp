#include "deviceconnectdialog.h"
#include "ui_deviceconnectdialog.h"
#include "core/device/devicemanager.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>

DeviceConnectDialog::DeviceConnectDialog(DeviceManager *deviceManager, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceConnectDialog)
    , m_deviceManager(deviceManager)
{
    ui->setupUi(this);
    setupConnections();
    
    // 设置按钮文本
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("连接"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    
    // 初始化时刷新设备列表
    refreshDeviceList();
    updateButtonState();
}

DeviceConnectDialog::~DeviceConnectDialog()
{
    delete ui;
}

QString DeviceConnectDialog::getSelectedDeviceUdid() const
{
    return m_selectedUdid;
}

QString DeviceConnectDialog::getSelectedDeviceName() const
{
    return m_selectedName;
}

void DeviceConnectDialog::setupConnections()
{
    // 连接设备管理器信号
    connect(m_deviceManager, &DeviceManager::deviceFound,
            this, &DeviceConnectDialog::onDeviceFound);
    connect(m_deviceManager, &DeviceManager::deviceLost,
            this, &DeviceConnectDialog::onDeviceLost);
    connect(m_deviceManager, &DeviceManager::deviceConnected,
            this, &DeviceConnectDialog::onDeviceConnected);
    connect(m_deviceManager, &DeviceManager::deviceDisconnected,
            this, &DeviceConnectDialog::onDeviceDisconnected);
    connect(m_deviceManager, &DeviceManager::errorOccurred,
            this, &DeviceConnectDialog::onDeviceError);
    connect(m_deviceManager, &DeviceManager::noDevicesFound,
            this, &DeviceConnectDialog::onNoDevicesFound);
    
    // 连接UI信号
    connect(ui->refreshButton, &QPushButton::clicked,
            this, &DeviceConnectDialog::onRefreshClicked);
    connect(ui->deviceListWidget, &QListWidget::itemSelectionChanged,
            this, &DeviceConnectDialog::onDeviceSelectionChanged);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &DeviceConnectDialog::onConnectClicked);
    connect(ui->deviceListWidget, &QListWidget::itemDoubleClicked,
            this, &DeviceConnectDialog::onConnectClicked);
}

void DeviceConnectDialog::refreshDeviceList()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    
    if (!loader.isInitialized()) {
        updateStatus(tr("libimobiledevice 未安装，无法连接 iOS 设备"));
        return;
    }
    
    ui->deviceListWidget->clear();
    updateStatus(tr("正在搜索设备..."), true);
    
    // 手动填充已知设备列表（不调用 refreshDevices 避免重复信号）
    const QStringList& devices = m_deviceManager->getConnectedDevices();
    for (const QString& udid : devices) {
        QString deviceName = m_deviceManager->getDeviceName(udid);
        if (deviceName.isEmpty() || deviceName == "Unknown Device") {
            deviceName = "iOS Device";
        }
        QListWidgetItem *item = new QListWidgetItem(QString("%1\n%2").arg(deviceName, udid));
        item->setData(Qt::UserRole, udid);
        item->setData(Qt::UserRole + 1, deviceName);
        ui->deviceListWidget->addItem(item);
    }
    
    if (devices.isEmpty()) {
        // 只有在没有设备时才刷新（可能是第一次打开）
        m_deviceManager->refreshDevices();
        
        // 刷新后再次检查
        const QStringList& devicesAfterRefresh = m_deviceManager->getConnectedDevices();
        for (const QString& udid : devicesAfterRefresh) {
            // 检查是否已存在
            bool exists = false;
            for (int i = 0; i < ui->deviceListWidget->count(); ++i) {
                if (ui->deviceListWidget->item(i)->data(Qt::UserRole).toString() == udid) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                QString deviceName = m_deviceManager->getDeviceName(udid);
                if (deviceName.isEmpty() || deviceName == "Unknown Device") {
                    deviceName = "iOS Device";
                }
                QListWidgetItem *item = new QListWidgetItem(QString("%1\n%2").arg(deviceName, udid));
                item->setData(Qt::UserRole, udid);
                item->setData(Qt::UserRole + 1, deviceName);
                ui->deviceListWidget->addItem(item);
            }
        }
        
        if (ui->deviceListWidget->count() == 0) {
            updateStatus(tr("未发现 iOS 设备"));
        } else {
            updateStatus(tr("找到 %1 台设备").arg(ui->deviceListWidget->count()));
        }
    } else {
        updateStatus(tr("找到 %1 台设备").arg(devices.size()));
    }
    
    updateButtonState();
}

void DeviceConnectDialog::updateStatus(const QString &message, bool showProgress)
{
    ui->statusLabel->setText(message);
    ui->progressBar->setVisible(showProgress);
}

void DeviceConnectDialog::updateButtonState()
{
    bool hasSelection = ui->deviceListWidget->currentItem() != nullptr;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelection);
}

void DeviceConnectDialog::onDeviceFound(const QString &udid, const QString &name)
{
    qDebug() << "Dialog: 发现设备" << udid << name;
    
    // 检查是否已存在
    for (int i = 0; i < ui->deviceListWidget->count(); ++i) {
        if (ui->deviceListWidget->item(i)->data(Qt::UserRole).toString() == udid) {
            return;
        }
    }
    
    QListWidgetItem *item = new QListWidgetItem(QString("%1\n%2").arg(name, udid));
    item->setData(Qt::UserRole, udid);
    item->setData(Qt::UserRole + 1, name);
    ui->deviceListWidget->addItem(item);
    
    updateStatus(tr("找到 %1 台设备").arg(ui->deviceListWidget->count()));
    updateButtonState();
}

void DeviceConnectDialog::onDeviceLost(const QString &udid)
{
    qDebug() << "Dialog: 设备断开" << udid;
    
    for (int i = 0; i < ui->deviceListWidget->count(); ++i) {
        QListWidgetItem *item = ui->deviceListWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == udid) {
            delete ui->deviceListWidget->takeItem(i);
            break;
        }
    }
    
    updateStatus(tr("找到 %1 台设备").arg(ui->deviceListWidget->count()));
    updateButtonState();
}

void DeviceConnectDialog::onDeviceConnected(const QString &udid)
{
    qDebug() << "Dialog: 设备已连接" << udid;
    updateStatus(tr("已连接到设备"));
    
    // 连接成功，关闭对话框
    accept();
}

void DeviceConnectDialog::onDeviceDisconnected()
{
    qDebug() << "Dialog: 设备已断开连接";
    updateStatus(tr("设备已断开连接"));
}

void DeviceConnectDialog::onDeviceError(const QString &error)
{
    qDebug() << "Dialog: 设备错误" << error;
    updateStatus(tr("错误: %1").arg(error));
    QMessageBox::warning(this, tr("连接错误"), error);
}

void DeviceConnectDialog::onNoDevicesFound()
{
    qDebug() << "Dialog: 未发现任何设备";
    updateStatus(tr("未发现 iOS 设备\n请确保设备已连接并已信任此电脑"));
}

void DeviceConnectDialog::onRefreshClicked()
{
    refreshDeviceList();
}

void DeviceConnectDialog::onDeviceSelectionChanged()
{
    QListWidgetItem *current = ui->deviceListWidget->currentItem();
    if (current) {
        m_selectedUdid = current->data(Qt::UserRole).toString();
        m_selectedName = current->data(Qt::UserRole + 1).toString();
    } else {
        m_selectedUdid.clear();
        m_selectedName.clear();
    }
    updateButtonState();
}

void DeviceConnectDialog::onConnectClicked()
{
    QListWidgetItem *current = ui->deviceListWidget->currentItem();
    if (!current) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一台设备"));
        return;
    }
    
    QString udid = current->data(Qt::UserRole).toString();
    m_selectedUdid = udid;
    m_selectedName = current->data(Qt::UserRole + 1).toString();
    
    updateStatus(tr("正在连接设备..."), true);
    
    // 连接设备
    m_deviceManager->connectToDevice(udid);
}