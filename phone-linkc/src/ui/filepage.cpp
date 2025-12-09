/**
 * @file filepage.cpp
 * @brief 文件管理页面实现
 */

#include "filepage.h"
#include "ui_filepage.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

FilePage::FilePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FilePage)
    , m_fileManager(nullptr)
{
    ui->setupUi(this);
    setupUI();
}

FilePage::~FilePage()
{
    delete ui;
}

void FilePage::setupUI()
{
    // 初始化侧边栏
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(ui->sidebarTree);
    rootItem->setText(0, "文件系统");
    rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DriveHDIcon));
    rootItem->setData(0, Qt::UserRole, "/");
    ui->sidebarTree->expandAll();

    // 设置文件列表头
    ui->fileList->setColumnWidth(0, 300); // 名称
    ui->fileList->setColumnWidth(1, 100); // 类型
    ui->fileList->setColumnWidth(2, 100); // 大小
    ui->fileList->setColumnWidth(3, 150); // 修改日期

    // 连接信号
    connect(ui->sidebarTree, &QTreeWidget::itemClicked, this, &FilePage::onSidebarItemClicked);
    connect(ui->fileList, &QTreeWidget::itemDoubleClicked, this, &FilePage::onFileItemDoubleClicked);
    
    connect(ui->btnRefresh, &QPushButton::clicked, this, &FilePage::refresh);
    connect(ui->btnImport, &QPushButton::clicked, this, &FilePage::onImportClicked);
    connect(ui->btnExport, &QPushButton::clicked, this, &FilePage::onExportClicked);
    connect(ui->btnNewFolder, &QPushButton::clicked, this, &FilePage::onNewFolderClicked);
    connect(ui->btnDelete, &QPushButton::clicked, this, &FilePage::onDeleteClicked);
}

void FilePage::setFileManager(FileManager *manager)
{
    if (m_fileManager) {
        disconnect(m_fileManager, nullptr, this, nullptr);
    }
    m_fileManager = manager;
    if (m_fileManager) {
        connect(m_fileManager, &FileManager::errorOccurred, this, &FilePage::onErrorOccurred);
    }
}

void FilePage::setCurrentDevice(const QString &udid)
{
    m_currentUdid = udid;
    m_currentPath = "/";
    refresh();
}

void FilePage::clearDevice()
{
    m_currentUdid.clear();
    m_currentPath.clear();
    ui->fileList->clear();
    ui->pathEdit->clear();
}

void FilePage::refresh()
{
    if (m_currentUdid.isEmpty() || !m_fileManager) {
        return;
    }
    
    // 如果未连接，尝试连接
    if (!m_fileManager->isConnected()) {
        if (!m_fileManager->connectToDevice(m_currentUdid)) {
            return;
        }
    }
    
    loadDirectory(m_currentPath);
}

void FilePage::loadDirectory(const QString &path)
{
    if (!m_fileManager) return;
    
    ui->pathEdit->setText(path);
    ui->fileList->clear();
    
    QVector<FileNode> nodes = m_fileManager->listDirectory(path);
    
    for (const FileNode &node : nodes) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->fileList);
        item->setText(0, node.name);
        item->setText(1, node.isDir ? "文件夹" : "文件");
        item->setText(2, node.isDir ? "-" : formatFileSize(node.size));
        item->setText(3, node.modifiedTime.toString("yyyy-MM-dd HH:mm:ss"));
        
        item->setIcon(0, style()->standardIcon(node.isDir ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
        
        // 存储完整路径
        QString fullPath = path;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += node.name;
        item->setData(0, Qt::UserRole, fullPath);
        item->setData(0, Qt::UserRole + 1, node.isDir);
    }
}

void FilePage::onSidebarItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        m_currentPath = path;
        loadDirectory(path);
    }
}

void FilePage::onFileItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    QString path = item->data(0, Qt::UserRole).toString();
    
    if (isDir) {
        m_currentPath = path;
        loadDirectory(path);
    }
}

void FilePage::onImportClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择要导入的文件");
    if (files.isEmpty()) return;
    
    // TODO: 实现导入逻辑
    QMessageBox::information(this, "提示", "导入功能暂未实现");
}

void FilePage::onExportClicked()
{
    QList<QTreeWidgetItem*> items = ui->fileList->selectedItems();
    if (items.isEmpty()) return;
    
    QString dir = QFileDialog::getExistingDirectory(this, "选择导出目录");
    if (dir.isEmpty()) return;
    
    for (QTreeWidgetItem *item : items) {
        bool isDir = item->data(0, Qt::UserRole + 1).toBool();
        if (isDir) continue; // 暂不支持导出目录
        
        QString path = item->data(0, Qt::UserRole).toString();
        QString fileName = item->text(0);
        QByteArray data = m_fileManager->readFile(path);
        
        if (!data.isEmpty()) {
            QFile file(dir + "/" + fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
            }
        }
    }
    QMessageBox::information(this, "完成", "导出完成");
}

void FilePage::onNewFolderClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "新建文件夹",
                                         "文件夹名称:", QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        QString newPath = m_currentPath;
        if (!newPath.endsWith("/")) newPath += "/";
        newPath += text;
        
        if (m_fileManager->createDirectory(newPath)) {
            refresh();
        }
    }
}

void FilePage::onDeleteClicked()
{
    QList<QTreeWidgetItem*> items = ui->fileList->selectedItems();
    if (items.isEmpty()) return;
    
    if (QMessageBox::question(this, "确认", "确定要删除选中的项目吗？") != QMessageBox::Yes) {
        return;
    }
    
    for (QTreeWidgetItem *item : items) {
        QString path = item->data(0, Qt::UserRole).toString();
        m_fileManager->deletePath(path);
    }
    refresh();
}

void FilePage::onErrorOccurred(const QString &error)
{
    QMessageBox::warning(this, "错误", error);
}

QString FilePage::formatFileSize(qint64 size)
{
    if (size < 1024) return QString("%1 B").arg(size);
    if (size < 1024 * 1024) return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    if (size < 1024 * 1024 * 1024) return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}