#include "contactpage.h"
#include "ui_contactpage.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

ContactPage::ContactPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ContactPage)
    , m_contactManager(nullptr)
{
    ui->setupUi(this);
    
    // 连接信号
    connect(ui->syncButton, &QPushButton::clicked,
            this, &ContactPage::onSyncButtonClicked);
    connect(ui->exportButton, &QPushButton::clicked,
            this, &ContactPage::onExportButtonClicked);
    connect(ui->searchEdit, &QLineEdit::textChanged,
            this, &ContactPage::onSearchTextChanged);
    connect(ui->contactTable, &QTableWidget::cellClicked,
            this, &ContactPage::onContactItemClicked);
    
    // 设置表格列宽
    ui->contactTable->setColumnWidth(0, 150);
    ui->contactTable->setColumnWidth(1, 150);
    
    // 初始状态
    setControlsEnabled(false);
    ui->detailText->setPlainText("请先连接设备并同步通讯录");
}

ContactPage::~ContactPage()
{
    delete ui;
}

void ContactPage::setContactManager(ContactManager* manager)
{
    // 断开旧连接
    if (m_contactManager) {
        disconnect(m_contactManager, nullptr, this, nullptr);
    }
    
    m_contactManager = manager;
    
    // 连接新信号
    if (m_contactManager) {
        connect(m_contactManager, &ContactManager::syncProgress,
                this, &ContactPage::onSyncProgress);
        connect(m_contactManager, &ContactManager::syncCompleted,
                this, &ContactPage::onSyncCompleted);
        connect(m_contactManager, &ContactManager::errorOccurred,
                this, [this](const QString& error) {
                    QMessageBox::warning(this, "错误", error);
                    setControlsEnabled(true);
                    ui->progressBar->setVisible(false);
                });
    }
}

void ContactPage::setCurrentDevice(const QString& udid)
{
    m_currentUdid = udid;
    
    if (!udid.isEmpty() && m_contactManager) {
        setControlsEnabled(true);
        ui->detailText->setPlainText("设备已连接，点击【同步通讯录】按钮开始同步");
    }
}

void ContactPage::clearDevice()
{
    m_currentUdid.clear();
    m_currentContacts.clear();
    
    ui->contactTable->setRowCount(0);
    ui->countLabel->setText("联系人: 0");
    ui->detailText->setPlainText("设备已断开连接");
    
    setControlsEnabled(false);
}

void ContactPage::onSyncButtonClicked()
{
    if (!m_contactManager || m_currentUdid.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先连接设备");
        return;
    }
    
    // 连接到设备
    if (!m_contactManager->connectToDevice(m_currentUdid)) {
        QMessageBox::warning(this, "错误", "无法连接到设备");
        return;
    }
    
    // 禁用控件
    setControlsEnabled(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
    ui->detailText->setPlainText("正在同步通讯录，请稍候...");
    
    // 开始同步
    m_contactManager->syncContacts();
}

void ContactPage::onSearchTextChanged(const QString& text)
{
    if (!m_contactManager) {
        return;
    }
    
    // 搜索联系人
    QVector<Contact> results;
    if (text.isEmpty()) {
        results = m_contactManager->getContacts();
    } else {
        results = m_contactManager->searchContacts(text);
    }
    
    updateContactList(results);
}

void ContactPage::onExportButtonClicked()
{
    if (!m_contactManager || m_contactManager->getContacts().isEmpty()) {
        QMessageBox::warning(this, "警告", "没有可导出的联系人");
        return;
    }
    
    // 选择保存路径
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出通讯录",
        defaultPath + "/contacts.vcf",
        "vCard 文件 (*.vcf);;所有文件 (*.*)"
    );
    
    if (filePath.isEmpty()) {
        return;
    }
    
    // 导出
    if (m_contactManager->exportToVCard(filePath)) {
        QMessageBox::information(this, "成功", 
            QString("已导出 %1 个联系人到:\n%2")
                .arg(m_contactManager->getContacts().size())
                .arg(filePath));
    } else {
        QMessageBox::warning(this, "错误", "导出失败");
    }
}

void ContactPage::onSyncProgress(int current, int total)
{
    if (total > 0) {
        ui->progressBar->setMaximum(total);
        ui->progressBar->setValue(current);
    }
}

void ContactPage::onSyncCompleted(bool success, const QString& message)
{
    setControlsEnabled(true);
    ui->progressBar->setVisible(false);
    
    if (success) {
        // 更新联系人列表
        m_currentContacts = m_contactManager->getContacts();
        updateContactList(m_currentContacts);
        
        ui->detailText->setPlainText(message);
        QMessageBox::information(this, "成功", message);
    } else {
        ui->detailText->setPlainText("同步失败: " + message);
        QMessageBox::warning(this, "失败", message);
    }
}

void ContactPage::onContactItemClicked(int row, int column)
{
    Q_UNUSED(column);
    
    if (row < 0 || row >= m_currentContacts.size()) {
        return;
    }
    
    const Contact& contact = m_currentContacts[row];
    showContactDetails(contact);
}

void ContactPage::updateContactList(const QVector<Contact>& contacts)
{
    m_currentContacts = contacts;
    
    // 更新计数
    ui->countLabel->setText(QString("联系人: %1").arg(contacts.size()));
    
    // 清空表格
    ui->contactTable->setRowCount(0);
    
    // 填充表格
    for (int i = 0; i < contacts.size(); ++i) {
        const Contact& contact = contacts[i];
        
        ui->contactTable->insertRow(i);
        
        // 姓名
        ui->contactTable->setItem(i, 0, new QTableWidgetItem(contact.fullName()));
        
        // 电话
        QString phones = contact.phoneNumbers.isEmpty() ? "" : contact.phoneNumbers.join(", ");
        ui->contactTable->setItem(i, 1, new QTableWidgetItem(phones));
        
        // 邮箱
        QString emails = contact.emails.isEmpty() ? "" : contact.emails.join(", ");
        ui->contactTable->setItem(i, 2, new QTableWidgetItem(emails));
    }
    
    // 如果有结果，显示第一个联系人详情
    if (!contacts.isEmpty()) {
        showContactDetails(contacts[0]);
        ui->contactTable->selectRow(0);
    } else {
        ui->detailText->setPlainText("未找到联系人");
    }
}

void ContactPage::showContactDetails(const Contact& contact)
{
    QString details;
    
    // 姓名
    details += "姓名: " + contact.fullName() + "\n";
    
    // 组织
    if (!contact.organization.isEmpty()) {
        details += "组织: " + contact.organization + "\n";
    }
    
    // 电话
    if (!contact.phoneNumbers.isEmpty()) {
        details += "\n电话号码:\n";
        for (const QString& phone : contact.phoneNumbers) {
            details += "  " + phone + "\n";
        }
    }
    
    // 邮箱
    if (!contact.emails.isEmpty()) {
        details += "\n邮箱地址:\n";
        for (const QString& email : contact.emails) {
            details += "  " + email + "\n";
        }
    }
    
    // 备注
    if (!contact.note.isEmpty()) {
        details += "\n备注:\n" + contact.note + "\n";
    }
    
    ui->detailText->setPlainText(details);
}

void ContactPage::setControlsEnabled(bool enabled)
{
    ui->syncButton->setEnabled(enabled);
    ui->exportButton->setEnabled(enabled && !m_currentContacts.isEmpty());
    ui->searchEdit->setEnabled(enabled && !m_currentContacts.isEmpty());
}