#ifndef CONTACTPAGE_H
#define CONTACTPAGE_H

#include <QWidget>
#include <QVector>
#include "core/contact/contactmanager.h"

namespace Ui {
class ContactPage;
}

class ContactPage : public QWidget
{
    Q_OBJECT

public:
    explicit ContactPage(QWidget *parent = nullptr);
    ~ContactPage();
    
    /**
     * @brief 设置通讯录管理器
     */
    void setContactManager(ContactManager* manager);
    
    /**
     * @brief 设置当前设备
     */
    void setCurrentDevice(const QString& udid);
    
    /**
     * @brief 清空设备
     */
    void clearDevice();

private slots:
    /**
     * @brief 同步按钮点击
     */
    void onSyncButtonClicked();
    
    /**
     * @brief 搜索文本改变
     */
    void onSearchTextChanged(const QString& text);
    
    /**
     * @brief 导出按钮点击
     */
    void onExportButtonClicked();
    
    /**
     * @brief 同步进度更新
     */
    void onSyncProgress(int current, int total);
    
    /**
     * @brief 同步完成
     */
    void onSyncCompleted(bool success, const QString& message);
    
    /**
     * @brief 联系人项被点击
     */
    void onContactItemClicked(int row, int column);
    
private:
    Ui::ContactPage *ui;
    ContactManager* m_contactManager;
    QString m_currentUdid;
    QVector<Contact> m_currentContacts;
    
    /**
     * @brief 更新联系人列表显示
     */
    void updateContactList(const QVector<Contact>& contacts);
    
    /**
     * @brief 显示联系人详情
     */
    void showContactDetails(const Contact& contact);
    
    /**
     * @brief 启用/禁用控件
     */
    void setControlsEnabled(bool enabled);
};

#endif // CONTACTPAGE_H