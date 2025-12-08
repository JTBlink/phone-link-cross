#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_deviceManager(new DeviceManager(this))
    , m_infoManager(new DeviceInfoManager(this))
{
    ui->setupUi(this);
    setupUI();
    
    // 连接设备管理器信号
    connect(m_deviceManager, &DeviceManager::deviceFound,
            this, &MainWindow::onDeviceFound);
    connect(m_deviceManager, &DeviceManager::deviceLost,
            this, &MainWindow::onDeviceLost);
    connect(m_deviceManager, &DeviceManager::deviceConnected,
            this, &MainWindow::onDeviceConnected);
    connect(m_deviceManager, &DeviceManager::deviceDisconnected,
            this, &MainWindow::onDeviceDisconnected);
    connect(m_deviceManager, &DeviceManager::errorOccurred,
            this, &MainWindow::onDeviceError);
    connect(m_deviceManager, &DeviceManager::noDevicesFound,
            this, &MainWindow::onNoDevicesFound);
    
    // 启动设备发现
    m_deviceManager->startDiscovery();
    
    setWindowTitle("iOS 设备管理器 - libimobiledevice 示例");
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    if (m_deviceManager) {
        m_deviceManager->stopDiscovery();
    }
    delete ui;
}

void MainWindow::setupUI()
{
    // 连接设备列表信号
    connect(ui->deviceList, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onDeviceSelectionChanged);
    
    // 连接按钮信号
    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectButtonClicked);
    connect(ui->refreshButton, &QPushButton::clicked,
            this, &MainWindow::onRefreshButtonClicked);
    connect(ui->getInfoButton, &QPushButton::clicked,
            this, &MainWindow::onGetInfoButtonClicked);
    
    // 设置分割器拉伸因子
    ui->mainSplitter->setStretchFactor(0, 1);
    ui->mainSplitter->setStretchFactor(1, 2);
    
    // 将状态标签添加到状态栏
    statusBar()->addWidget(ui->statusLabel);
    
    // 初始化状态标签并根据运行时动态库加载状态设置初始文本
    updateDisplayText(getInitialInfoText(), getInitialStatusText());
    
    // 初始状态
    updateConnectionStatus();
}

void MainWindow::onDeviceFound(const QString &udid, const QString &name)
{
    qDebug() << "UI: 发现设备" << udid << name;
    
    QListWidgetItem *item = new QListWidgetItem(QString("%1\n%2").arg(name, udid));
    item->setData(Qt::UserRole, udid);
    ui->deviceList->addItem(item);
    
    updateDisplayText(QString(), QString("找到 %1 台设备").arg(ui->deviceList->count()));
}

void MainWindow::onDeviceLost(const QString &udid)
{
    qDebug() << "UI: 设备断开连接" << udid;
    
    // 从列表中移除设备
    for (int i = 0; i < ui->deviceList->count(); ++i) {
        QListWidgetItem *item = ui->deviceList->item(i);
        if (item && item->data(Qt::UserRole).toString() == udid) {
            delete ui->deviceList->takeItem(i);
            break;
        }
    }
    
    updateDisplayText(QString(), QString("找到 %1 台设备").arg(ui->deviceList->count()));
}

void MainWindow::onDeviceConnected(const QString &udid)
{
    qDebug() << "UI: 设备已连接" << udid;
    updateDisplayText(QString(), QString("已连接到设备: %1").arg(udid));
    updateConnectionStatus();
    updateDeviceInfo(udid);
}

void MainWindow::onDeviceDisconnected()
{
    qDebug() << "UI: 设备已断开连接";
    updateDisplayText("设备已断开连接", "设备已断开连接");
    updateConnectionStatus();
}

void MainWindow::onDeviceError(const QString &error)
{
    qDebug() << "UI: 设备错误" << error;
    QMessageBox::warning(this, "设备错误", error);
    updateDisplayText(QString(), QString("错误: %1").arg(error));
}

void MainWindow::onNoDevicesFound()
{
    qDebug() << "UI: 未发现任何设备";
    updateDisplayText(getNoDeviceInfoText(), "未发现 iOS 设备");
}

void MainWindow::onDeviceSelectionChanged()
{
    QListWidgetItem *current = ui->deviceList->currentItem();
    if (current) {
        QString udid = current->data(Qt::UserRole).toString();
        if (!m_deviceManager->isConnected() ||
            m_deviceManager->getCurrentDevice() != udid) {
            updateDisplayText(QString("选中设备: %1\n点击 '连接设备' 或 '获取信息' 按钮").arg(udid));
        }
    }
    updateConnectionStatus();
}

void MainWindow::onConnectButtonClicked()
{
    QListWidgetItem *current = ui->deviceList->currentItem();
    if (!current) {
        QMessageBox::information(this, "提示", "请先选择一台设备");
        return;
    }
    
    QString udid = current->data(Qt::UserRole).toString();
    
    if (m_deviceManager->isConnected() && m_deviceManager->getCurrentDevice() == udid) {
        // 断开连接
        m_deviceManager->disconnectFromDevice();
    } else {
        // 连接设备
        m_deviceManager->connectToDevice(udid);
    }
}

void MainWindow::onRefreshButtonClicked()
{
    qDebug() << "手动刷新设备列表";
    
    // 检查运行时动态库加载状态
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    bool isLibraryAvailable = loader.isInitialized();
    
    if (isLibraryAvailable) {
        updateDisplayText(QString(), "正在搜索 iOS 设备...");
        
        // 先清空UI列表
        ui->deviceList->clear();
        
        // 调用设备管理器的刷新方法
        m_deviceManager->refreshDevices();
        
        // 处理完刷新后，根据结果重新填充界面
        QApplication::processEvents(); // 让事件处理完成
        
        // 手动重新填充设备列表
        const QStringList& devices = m_deviceManager->getConnectedDevices();
        for (const QString& udid : devices) {
            // 获取设备名称并添加到列表
            QString deviceName = "iOS Device"; // 默认名称
            // 这里可以调用 getDeviceName 获取实际名称，但为了避免阻塞UI，使用简单名称
            QListWidgetItem *item = new QListWidgetItem(QString("%1\n%2").arg(deviceName, udid));
            item->setData(Qt::UserRole, udid);
            ui->deviceList->addItem(item);
        }
        
        if (devices.isEmpty()) {
            updateDisplayText(getNoDeviceInfoText(), "未发现 iOS 设备");
        } else {
            updateDisplayText(QString(), QString("找到 %1 台设备").arg(devices.size()));
        }
    } else {
        ui->deviceList->clear();
        updateDisplayText(getLibraryNotInstalledInfoText(),
                         "libimobiledevice 未安装 - 无法连接 iOS 设备");
    }
}

void MainWindow::onGetInfoButtonClicked()
{
    QListWidgetItem *current = ui->deviceList->currentItem();
    if (!current) {
        QMessageBox::information(this, "提示", "请先选择一台设备");
        return;
    }
    
    QString udid = current->data(Qt::UserRole).toString();
    updateDeviceInfo(udid);
}

void MainWindow::updateDeviceInfo(const QString &udid)
{
    ui->infoDisplay->setPlainText("正在获取设备信息...");
    
    // 在后台线程获取设备信息
    QApplication::processEvents();
    
    DeviceInfo info = m_infoManager->getDeviceInfo(udid);
    
    QString infoText = QString(
        "========== iOS 设备信息 ==========\n\n"
        "设备名称: %1\n"
        "UDID: %2\n"
        "型号: %3\n"
        "系统版本: %4 (%5)\n"
        "序列号: %6\n"
        "设备类型: %7\n"
        "产品类型: %8\n"
        "总容量: %9 GB\n"
        "可用容量: %10 GB\n"
        "WiFi地址: %11\n"
        "激活状态: %12\n\n"
        "========== libimobiledevice 状态 ==========\n"
        "连接状态: %13\n"
        "支持库: %14"
    ).arg(info.name.isEmpty() ? "未知" : info.name)
     .arg(info.udid)
     .arg(info.model.isEmpty() ? "未知" : info.model)
     .arg(info.productVersion.isEmpty() ? "未知" : info.productVersion)
     .arg(info.buildVersion.isEmpty() ? "未知" : info.buildVersion)
     .arg(info.serialNumber.isEmpty() ? "未知" : info.serialNumber)
     .arg(info.deviceClass.isEmpty() ? "未知" : info.deviceClass)
     .arg(info.productType.isEmpty() ? "未知" : info.productType)
     .arg(info.totalCapacity > 0 ? QString::number(info.totalCapacity / (1024*1024*1024)) : "未知")
     .arg(info.availableCapacity > 0 ? QString::number(info.availableCapacity / (1024*1024*1024)) : "未知")
     .arg(info.wifiAddress.isEmpty() ? "未知" : info.wifiAddress)
     .arg(info.activationState.isEmpty() ? "未知" : info.activationState)
     .arg(m_deviceManager->isConnected() ? "已连接" : "未连接")
     .arg(LibimobiledeviceDynamic::instance().isInitialized() ?
          "libimobiledevice (动态加载 - 真实设备支持)" :
          "libimobiledevice 未安装，无法连接设备");
    
    ui->infoDisplay->setPlainText(infoText);
}

void MainWindow::updateConnectionStatus()
{
    QListWidgetItem *current = ui->deviceList->currentItem();
    bool hasSelection = current != nullptr;
    bool isConnected = m_deviceManager->isConnected();
    
    if (hasSelection && isConnected) {
        QString currentUdid = current->data(Qt::UserRole).toString();
        bool isCurrentDevice = (currentUdid == m_deviceManager->getCurrentDevice());
        ui->connectButton->setText(isCurrentDevice ? "断开连接" : "连接设备");
        ui->connectButton->setEnabled(true);
    } else {
        ui->connectButton->setText("连接设备");
        ui->connectButton->setEnabled(hasSelection);
    }
    
    ui->getInfoButton->setEnabled(hasSelection);
}

void MainWindow::updateDisplayText(const QString &infoText,
                                   const QString &statusText,
                                   bool createStatusLabel)
{
    Q_UNUSED(createStatusLabel) // 保留参数以兼容旧代码，但不再使用
    
    if (!infoText.isEmpty()) {
        ui->infoDisplay->setPlainText(infoText);
    }
    
    if (!statusText.isEmpty()) {
        ui->statusLabel->setText(statusText);
    }
}

QString MainWindow::getNoDeviceInfoText() const
{
    return "未发现 iOS 设备\n\n"
           "请确保：\n"
           "1. iOS 设备已连接到电脑\n"
           "2. 设备已解锁并信任此电脑\n"
           "3. iTunes 或其他同步软件已关闭\n\n"
           "连接设备后点击【刷新列表】按钮重新检测";
}

QString MainWindow::getLibraryNotInstalledInfoText() const
{
    return "libimobiledevice 未安装\n\n"
           "无法连接 iOS 设备。\n\n"
           "请安装 libimobiledevice 以支持 iOS 设备连接。\n\n"
           "安装方法：\n"
           "• macOS: brew install libimobiledevice\n"
           "• Ubuntu: sudo apt install libimobiledevice-utils\n"
           "• Windows: 请参考项目文档";
}

QString MainWindow::getInitialInfoText() const
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    return loader.isInitialized() ? getNoDeviceInfoText() : getLibraryNotInstalledInfoText();
}

QString MainWindow::getInitialStatusText() const
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    return loader.isInitialized()
           ? "正在搜索 iOS 设备..."
           : "libimobiledevice 未安装 - 无法连接 iOS 设备";
}