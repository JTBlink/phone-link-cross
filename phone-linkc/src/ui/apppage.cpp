#include "apppage.h"
#include "ui_apppage.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QColor>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>

AppPage::AppPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AppPage)
    , m_appManager(nullptr)
{
    ui->setupUi(this);
    setupUI();
}

AppPage::~AppPage()
{
    delete ui;
}

void AppPage::setupUI()
{
    connect(ui->installButton, &QPushButton::clicked, this, &AppPage::onInstallClicked);
    connect(ui->uninstallButton, &QPushButton::clicked, this, &AppPage::onUninstallClicked);
    connect(ui->refreshButton, &QPushButton::clicked, this, &AppPage::onRefreshClicked);
    
    // 设置列宽
    ui->appList->setColumnWidth(0, 250); // Name
    ui->appList->setColumnWidth(1, 100); // Type
    ui->appList->setColumnWidth(2, 100); // Version
    ui->appList->setColumnWidth(3, 100); // App Size
    ui->appList->setColumnWidth(4, 100); // Doc Size
    // Bundle ID 自适应
}

void AppPage::setAppManager(AppManager *manager)
{
    m_appManager = manager;
    connect(m_appManager, &AppManager::errorOccurred, this, &AppPage::onErrorOccurred);
    connect(m_appManager, &AppManager::appsLoaded, this, &AppPage::onAppsLoaded);
    connect(m_appManager, &AppManager::appSizeUpdated, this, &AppPage::onAppSizeUpdated);
    connect(m_appManager, &AppManager::progressUpdated, this, &AppPage::onProgressUpdated);
}

void AppPage::setCurrentDevice(const QString &udid)
{
    m_currentUdid = udid;
    if (m_appManager) {
        m_appManager->connectToDevice(udid);
        refresh();
    }
}

void AppPage::clearDevice()
{
    m_currentUdid.clear();
    ui->appList->clear();
    if (m_appManager) {
        m_appManager->disconnectFromDevice();
    }
}

void AppPage::refresh()
{
    onRefreshClicked();
}

void AppPage::loadApps()
{
    if (!m_appManager || m_currentUdid.isEmpty()) return;
    
    ui->appList->clear();
    
    // 显示加载提示
    QTreeWidgetItem *loadingItem = new QTreeWidgetItem(ui->appList);
    loadingItem->setText(0, "正在加载应用列表，请稍候...");
    loadingItem->setForeground(0, QColor(128, 128, 128));
    
    // 禁用按钮
    ui->installButton->setEnabled(false);
    ui->uninstallButton->setEnabled(false);
    ui->refreshButton->setEnabled(false);
    
    // 异步加载
    m_appManager->listAppsAsync();
}

void AppPage::onAppsLoaded(const QVector<AppInfo> &apps)
{
    // 清空列表
    ui->appList->clear();
    
    // 重新启用按钮
    ui->installButton->setEnabled(true);
    ui->uninstallButton->setEnabled(true);
    ui->refreshButton->setEnabled(true);
    
    // 填充应用列表（基本信息）
    for (const AppInfo &app : apps) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->appList);
        item->setText(0, app.name);
        item->setText(1, app.type == "User" ? "用户应用" : "系统应用");
        item->setText(2, app.version);
        item->setText(3, app.appSize.isEmpty() ? "加载中..." : app.appSize);
        item->setText(4, app.docSize.isEmpty() ? "加载中..." : app.docSize);
        item->setText(5, app.bundleId);
        
        // 如果大小信息为空，设置灰色显示
        if (app.appSize.isEmpty()) {
            item->setForeground(3, QColor(128, 128, 128));
            item->setForeground(4, QColor(128, 128, 128));
        }
        // TODO: Set Icon
    }
    
    // 显示统计信息
    ui->appList->headerItem()->setText(0, QString("名称 (共 %1 个应用，大小信息加载中...)").arg(apps.size()));
}

void AppPage::onAppSizeUpdated(const QString &bundleId, const QString &appSize, const QString &docSize)
{
    // 查找对应的列表项并更新大小信息
    for (int i = 0; i < ui->appList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->appList->topLevelItem(i);
        if (item && item->text(5) == bundleId) {
            item->setText(3, appSize);
            item->setText(4, docSize);
            
            // 恢复正常颜色
            item->setForeground(3, QColor(0, 0, 0));
            item->setForeground(4, QColor(0, 0, 0));
            
            // 检查是否所有应用的大小都已加载
            bool allLoaded = true;
            for (int j = 0; j < ui->appList->topLevelItemCount(); ++j) {
                QTreeWidgetItem *checkItem = ui->appList->topLevelItem(j);
                if (checkItem && checkItem->text(3) == "加载中...") {
                    allLoaded = false;
                    break;
                }
            }
            
            // 如果全部加载完成，更新标题
            if (allLoaded) {
                ui->appList->headerItem()->setText(0, QString("名称 (共 %1 个应用)").arg(ui->appList->topLevelItemCount()));
            }
            
            break;
        }
    }
}

void AppPage::onInstallClicked()
{
    if (!m_appManager || m_currentUdid.isEmpty()) return;
    
    QString fileName = QFileDialog::getOpenFileName(this, "选择应用安装包",
                                                    "", "iOS 应用 (*.ipa)");
    if (fileName.isEmpty()) {
        return;
    }
    
    // 创建进度对话框
    QProgressDialog *progress = new QProgressDialog("正在准备安装...", "取消", 0, 100, this);
    progress->setWindowTitle("安装应用");
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);
    
    // 存储进度对话框指针，以便在进度更新时使用
    m_installProgress = progress;
    
    // 在新线程中执行安装
    QtConcurrent::run([this, fileName, progress]() {
        bool success = m_appManager->installApp(fileName);
        
        // 在主线程中处理结果
        QMetaObject::invokeMethod(this, [this, progress, success]() {
            if (progress) {
                progress->close();
                progress->deleteLater();
                m_installProgress = nullptr;
            }
            
            if (success) {
                QMessageBox::information(this, "成功", "应用安装成功");
                refresh();
            }
        }, Qt::QueuedConnection);
    });
}

void AppPage::onUninstallClicked()
{
    if (!m_appManager || m_currentUdid.isEmpty()) return;
    
    QTreeWidgetItem *item = ui->appList->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请选择要卸载的应用");
        return;
    }
    
    QString bundleId = item->text(5);
    QString name = item->text(0);
    
    if (QMessageBox::question(this, "确认卸载", 
                              QString("确定要卸载 %1 (%2) 吗？").arg(name).arg(bundleId)) 
        == QMessageBox::Yes) {
        if (m_appManager->uninstallApp(bundleId)) {
            QMessageBox::information(this, "成功", "应用已卸载");
            refresh();
        }
    }
}

void AppPage::onRefreshClicked()
{
    loadApps();
}

void AppPage::onErrorOccurred(const QString &error)
{
    // 关闭进度对话框（如果存在）
    if (m_installProgress) {
        m_installProgress->close();
        m_installProgress->deleteLater();
        m_installProgress = nullptr;
    }
    
    QMessageBox::warning(this, "错误", error);
}

void AppPage::onProgressUpdated(const QString &message, int percent)
{
    if (m_installProgress) {
        m_installProgress->setLabelText(message);
        m_installProgress->setValue(percent);
        
        // 如果用户点击了取消按钮
        if (m_installProgress->wasCanceled()) {
            // TODO: 实现取消安装的逻辑
            m_installProgress->close();
            m_installProgress->deleteLater();
            m_installProgress = nullptr;
        }
    }
}