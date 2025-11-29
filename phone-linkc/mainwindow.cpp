#include "mainwindow.h"
#include "ui_mainwindow.h"
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
    // 创建中央widget
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // 创建主分割器
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    
    // 左侧设备列表
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    
    // 设备列表
    m_deviceList = new QListWidget;
    connect(m_deviceList, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onDeviceSelectionChanged);
    leftLayout->addWidget(m_deviceList);
    
    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    m_connectButton = new QPushButton("连接设备");
    m_refreshButton = new QPushButton("刷新列表");
    m_getInfoButton = new QPushButton("获取信息");
    
    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectButtonClicked);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &MainWindow::onRefreshButtonClicked);
    connect(m_getInfoButton, &QPushButton::clicked,
            this, &MainWindow::onGetInfoButtonClicked);
    
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_getInfoButton);
    leftLayout->addLayout(buttonLayout);
    
    // 右侧信息显示
    m_infoDisplay = new QTextEdit;
    m_infoDisplay->setReadOnly(true);
#ifdef HAVE_LIBIMOBILEDEVICE
    m_infoDisplay->setPlainText("正在搜索 iOS 设备...\n\n请确保：\n1. iOS 设备已连接到电脑\n2. 设备已解锁并信任此电脑\n3. iTunes 或其他同步软件已关闭");
#else
    m_infoDisplay->setPlainText("libimobiledevice 未安装\n\n无法连接 iOS 设备。\n\n请安装 libimobiledevice 以支持 iOS 设备连接。\n\n安装方法：\n• macOS: brew install libimobiledevice\n• Ubuntu: sudo apt install libimobiledevice-utils\n• Windows: 请参考项目文档");
#endif
    
    // 添加到分割器
    m_mainSplitter->addWidget(leftWidget);
    m_mainSplitter->addWidget(m_infoDisplay);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 2);
    
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
    
    // 状态栏
#ifdef HAVE_LIBIMOBILEDEVICE
    m_statusLabel = new QLabel("正在搜索 iOS 设备...");
#else
    m_statusLabel = new QLabel("libimobiledevice 未安装 - 无法连接 iOS 设备");
#endif
    statusBar()->addWidget(m_statusLabel);
    
    // 初始状态
    updateConnectionStatus();
}

void MainWindow::onDeviceFound(const QString &udid, const QString &name)
{
    qDebug() << "UI: 发现设备" << udid << name;
    
    QListWidgetItem *item = new QListWidgetItem(QString("%1\n%2").arg(name, udid));
    item->setData(Qt::UserRole, udid);
    m_deviceList->addItem(item);
    
    m_statusLabel->setText(QString("找到 %1 台设备").arg(m_deviceList->count()));
}

void MainWindow::onDeviceLost(const QString &udid)
{
    qDebug() << "UI: 设备断开连接" << udid;
    
    // 从列表中移除设备
    for (int i = 0; i < m_deviceList->count(); ++i) {
        QListWidgetItem *item = m_deviceList->item(i);
        if (item && item->data(Qt::UserRole).toString() == udid) {
            delete m_deviceList->takeItem(i);
            break;
        }
    }
    
    m_statusLabel->setText(QString("找到 %1 台设备").arg(m_deviceList->count()));
}

void MainWindow::onDeviceConnected(const QString &udid)
{
    qDebug() << "UI: 设备已连接" << udid;
    m_statusLabel->setText(QString("已连接到设备: %1").arg(udid));
    updateConnectionStatus();
    updateDeviceInfo(udid);
}

void MainWindow::onDeviceDisconnected()
{
    qDebug() << "UI: 设备已断开连接";
    m_statusLabel->setText("设备已断开连接");
    m_infoDisplay->setPlainText("设备已断开连接");
    updateConnectionStatus();
}

void MainWindow::onDeviceError(const QString &error)
{
    qDebug() << "UI: 设备错误" << error;
    QMessageBox::warning(this, "设备错误", error);
    m_statusLabel->setText(QString("错误: %1").arg(error));
}

void MainWindow::onDeviceSelectionChanged()
{
    QListWidgetItem *current = m_deviceList->currentItem();
    if (current) {
        QString udid = current->data(Qt::UserRole).toString();
        if (!m_deviceManager->isConnected() || 
            m_deviceManager->getCurrentDevice() != udid) {
            m_infoDisplay->setPlainText(QString("选中设备: %1\n点击 '连接设备' 或 '获取信息' 按钮").arg(udid));
        }
    }
    updateConnectionStatus();
}

void MainWindow::onConnectButtonClicked()
{
    QListWidgetItem *current = m_deviceList->currentItem();
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
    m_deviceList->clear();
#ifdef HAVE_LIBIMOBILEDEVICE
    m_statusLabel->setText("正在搜索 iOS 设备...");
#else
    m_statusLabel->setText("libimobiledevice 未安装 - 无法连接 iOS 设备");
#endif
    // 设备管理器会自动发现设备
}

void MainWindow::onGetInfoButtonClicked()
{
    QListWidgetItem *current = m_deviceList->currentItem();
    if (!current) {
        QMessageBox::information(this, "提示", "请先选择一台设备");
        return;
    }
    
    QString udid = current->data(Qt::UserRole).toString();
    updateDeviceInfo(udid);
}

void MainWindow::updateDeviceInfo(const QString &udid)
{
    m_infoDisplay->setPlainText("正在获取设备信息...");
    
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
#ifdef HAVE_LIBIMOBILEDEVICE
     .arg("libimobiledevice (真实设备支持)");
#else
     .arg("libimobiledevice 未安装，无法连接设备");
#endif
    
    m_infoDisplay->setPlainText(infoText);
}

void MainWindow::updateConnectionStatus()
{
    QListWidgetItem *current = m_deviceList->currentItem();
    bool hasSelection = current != nullptr;
    bool isConnected = m_deviceManager->isConnected();
    
    if (hasSelection && isConnected) {
        QString currentUdid = current->data(Qt::UserRole).toString();
        bool isCurrentDevice = (currentUdid == m_deviceManager->getCurrentDevice());
        m_connectButton->setText(isCurrentDevice ? "断开连接" : "连接设备");
        m_connectButton->setEnabled(true);
    } else {
        m_connectButton->setText("连接设备");
        m_connectButton->setEnabled(hasSelection);
    }
    
    m_getInfoButton->setEnabled(hasSelection);
}
