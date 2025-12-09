#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "debugwindow.h"
#include "deviceconnectdialog.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_deviceManager(new DeviceManager(this))
    , m_infoManager(new DeviceInfoManager(this))
    , m_debugWindow(nullptr)
{
    ui->setupUi(this);
    setupUI();
    
    // 连接设备管理器信号
    connect(m_deviceManager, &DeviceManager::deviceConnected,
            this, &MainWindow::onDeviceConnected);
    connect(m_deviceManager, &DeviceManager::deviceDisconnected,
            this, &MainWindow::onDeviceDisconnected);
    connect(m_deviceManager, &DeviceManager::errorOccurred,
            this, &MainWindow::onDeviceError);
    
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
    // 连接按钮信号
    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectButtonClicked);
    connect(ui->disconnectButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectButtonClicked);
    
    // 连接调试菜单动作
    connect(ui->actionOpenDebugWindow, &QAction::triggered,
            this, &MainWindow::onOpenDebugWindow);
    
    // 连接菜单列表选择信号
    connect(ui->menuList, &QListWidget::currentRowChanged,
            this, &MainWindow::onMenuItemSelected);
    
    // 将状态标签添加到状态栏
    statusBar()->addWidget(ui->statusLabel);
    
    // 初始化状态标签并根据运行时动态库加载状态设置初始文本
    updateDisplayText(getInitialInfoText(), getInitialStatusText());
    
    // 默认选中第一项（设备信息）
    ui->menuList->setCurrentRow(0);
    
    // 初始状态
    updateConnectionStatus();
}

void MainWindow::onDeviceConnected(const QString &udid)
{
    qDebug() << "UI: 设备已连接" << udid;
    // 如果设备名称为空，使用默认名称
    if (m_connectedDeviceName.isEmpty()) {
        m_connectedDeviceName = "iOS Device";
    }
    
    ui->deviceNameLabel->setText(m_connectedDeviceName);
    ui->deviceTypeLabel->setText("已连接");
    updateDisplayText(QString(), QString("已连接到设备: %1").arg(m_connectedDeviceName));
    updateConnectionStatus();
    updateDeviceInfo(udid);
}

void MainWindow::onDeviceDisconnected()
{
    qDebug() << "UI: 设备已断开连接";
    m_connectedDeviceName.clear();
    ui->deviceNameLabel->setText("未连接设备");
    ui->deviceTypeLabel->setText("点击连接设备");
    updateDisplayText("设备已断开连接\n\n点击【连接设备】按钮连接新设备", "设备已断开连接");
    updateConnectionStatus();
}

void MainWindow::onDeviceError(const QString &error)
{
    qDebug() << "UI: 设备错误" << error;
    QMessageBox::warning(this, "设备错误", error);
    updateDisplayText(QString(), QString("错误: %1").arg(error));
}

void MainWindow::onConnectButtonClicked()
{
    // 打开设备连接对话框
    DeviceConnectDialog dialog(m_deviceManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString udid = dialog.getSelectedDeviceUdid();
        QString name = dialog.getSelectedDeviceName();
        if (!udid.isEmpty()) {
            m_connectedDeviceName = name.isEmpty() ? "iOS Device" : name;
            ui->deviceNameLabel->setText(m_connectedDeviceName);
            updateDeviceInfo(udid);
        }
    }
}

void MainWindow::onDisconnectButtonClicked()
{
    if (m_deviceManager->isConnected()) {
        m_deviceManager->disconnectFromDevice();
    }
}

void MainWindow::updateDeviceInfo(const QString &udid)
{
    ui->infoDisplay->setPlainText("正在获取设备信息...");
    
    // 在后台线程获取设备信息
    QApplication::processEvents();
    
    DeviceInfo info = m_infoManager->getDeviceInfo(udid);
    
    // 直接使用 DeviceInfo::toString() 方法获取格式化的设备信息
    ui->infoDisplay->setPlainText(info.toString());
}

void MainWindow::updateConnectionStatus()
{
    bool isConnected = m_deviceManager->isConnected();
    
    // 更新连接/断开按钮状态
    ui->connectButton->setEnabled(!isConnected);
    ui->disconnectButton->setEnabled(isConnected);
    
    // 更新菜单列表项状态（除了设备信息和更多，其他项需要设备连接才能使用）
    for (int i = 1; i < ui->menuList->count() - 1; ++i) {
        QListWidgetItem* item = ui->menuList->item(i);
        if (item) {
            item->setFlags(isConnected ?
                (Qt::ItemIsEnabled | Qt::ItemIsSelectable) :
                Qt::NoItemFlags);
        }
    }
}

void MainWindow::onMenuItemSelected(int index)
{
    // 切换到对应的页面
    if (index >= 0 && index < ui->contentStack->count()) {
        ui->contentStack->setCurrentIndex(index);
    }
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
    return "未连接设备\n\n"
           "点击【连接设备】按钮选择并连接 iOS 设备\n\n"
           "请确保：\n"
           "1. iOS 设备已连接到电脑\n"
           "2. 设备已解锁并信任此电脑\n"
           "3. iTunes 或其他同步软件已关闭";
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
           ? "就绪 - 点击连接设备开始"
           : "libimobiledevice 未安装 - 无法连接 iOS 设备";
}

void MainWindow::onOpenDebugWindow()
{
    if (!m_debugWindow) {
        m_debugWindow = new DebugWindow(this);
    }
    m_debugWindow->show();
    m_debugWindow->raise();
    m_debugWindow->activateWindow();
}
