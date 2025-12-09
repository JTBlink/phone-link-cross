/**
 * @file filepage.h
 * @brief 文件管理页面头文件
 */

#ifndef FILEPAGE_H
#define FILEPAGE_H

#include <QWidget>
#include <QTreeWidgetItem>
#include "core/file/filemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class FilePage;
}
QT_END_NAMESPACE

/**
 * @brief 文件管理页面类
 */
class FilePage : public QWidget
{
    Q_OBJECT

public:
    explicit FilePage(QWidget *parent = nullptr);
    ~FilePage();

    /**
     * @brief 设置文件管理器
     */
    void setFileManager(FileManager *manager);

    /**
     * @brief 设置当前设备UDID
     */
    void setCurrentDevice(const QString &udid);

    /**
     * @brief 清除当前设备状态
     */
    void clearDevice();

public slots:
    /**
     * @brief 刷新当前目录
     */
    void refresh();

private slots:
    /**
     * @brief 侧边栏项点击
     */
    void onSidebarItemClicked(QTreeWidgetItem *item, int column);

    /**
     * @brief 文件列表双击（进入目录）
     */
    void onFileItemDoubleClicked(QTreeWidgetItem *item, int column);

    /**
     * @brief 导入文件（上传到设备）
     */
    void onImportClicked();

    /**
     * @brief 导出文件（下载到本地）
     */
    void onExportClicked();

    /**
     * @brief 删除文件
     */
    void onDeleteClicked();

    /**
     * @brief 新建文件夹
     */
    void onNewFolderClicked();

    /**
     * @brief 发生错误
     */
    void onErrorOccurred(const QString &error);

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();

    /**
     * @brief 加载目录内容
     * @param path 目录路径
     */
    void loadDirectory(const QString &path);

    /**
     * @brief 格式化文件大小
     */
    QString formatFileSize(qint64 size);

    /**
     * @brief 获取选中的文件项列表
     */
    QList<QTreeWidgetItem*> getSelectedItems();

    Ui::FilePage *ui;
    FileManager *m_fileManager;
    QString m_currentUdid;
    QString m_currentPath;
};

#endif // FILEPAGE_H