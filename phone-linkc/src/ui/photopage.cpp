/**
 * @file photopage.cpp
 * @brief 照片页面组件实现
 */

#include "photopage.h"
#include "ui_photopage.h"
#include "flowlayout.h"

#include <QTreeWidgetItem>
#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QProgressDialog>
#include <QFile>
#include <QDir>

/* ============================================================================
 * PhotoThumbnail 实现
 * ============================================================================ */

PhotoThumbnail::PhotoThumbnail(const PhotoInfo &info, QWidget *parent)
    : QWidget(parent)
    , m_photoInfo(info)
    , m_selected(false)
    , m_hovered(false)
{
    setFixedSize(120, 120);
    setCursor(Qt::PointingHandCursor);
    
    // 设置默认缩略图（占位符）
    m_thumbnail = QPixmap(100, 100);
    m_thumbnail.fill(Qt::lightGray);
}

void PhotoThumbnail::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update();
        emit selectionChanged(selected);
    }
}

void PhotoThumbnail::setThumbnail(const QPixmap &pixmap)
{
    if (!pixmap.isNull()) {
        m_thumbnail = pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        update();
    }
}

void PhotoThumbnail::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 背景
    QRect bgRect = rect().adjusted(2, 2, -2, -2);
    if (m_selected) {
        painter.fillRect(bgRect, QColor("#e3f2fd"));
        painter.setPen(QPen(QColor("#1976d2"), 2));
        painter.drawRect(bgRect);
    } else if (m_hovered) {
        painter.fillRect(bgRect, QColor("#f5f5f5"));
        painter.setPen(QPen(QColor("#e0e0e0"), 1));
        painter.drawRect(bgRect);
    } else {
        painter.fillRect(bgRect, Qt::white);
        painter.setPen(QPen(QColor("#e8e8e8"), 1));
        painter.drawRect(bgRect);
    }
    
    // 缩略图
    int imgX = (width() - m_thumbnail.width()) / 2;
    int imgY = (height() - m_thumbnail.height()) / 2 - 8;
    painter.drawPixmap(imgX, imgY, m_thumbnail);
    
    // 文件名
    QRect textRect(4, height() - 24, width() - 8, 20);
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    QString elidedText = painter.fontMetrics().elidedText(
        m_photoInfo.name, Qt::ElideMiddle, textRect.width());
    painter.drawText(textRect, Qt::AlignCenter, elidedText);
    
    // 视频标识
    if (m_photoInfo.isVideo) {
        QRect videoRect(width() - 24, 6, 18, 18);
        painter.fillRect(videoRect, QColor(0, 0, 0, 128));
        painter.setPen(Qt::white);
        painter.drawText(videoRect, Qt::AlignCenter, "▶");
    }
    
    // 选中标记（心形图标）
    if (m_selected || m_hovered) {
        QRect heartRect(8, height() - 32, 16, 16);
        painter.setPen(m_selected ? QColor("#e65100") : QColor("#999999"));
        painter.setFont(QFont("Arial", 12));
        painter.drawText(heartRect, Qt::AlignCenter, m_selected ? "♥" : "♡");
    }
}

void PhotoThumbnail::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setSelected(!m_selected);
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void PhotoThumbnail::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void PhotoThumbnail::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void PhotoThumbnail::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

/* ============================================================================
 * PhotoPage 实现
 * ============================================================================ */

PhotoPage::PhotoPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PhotoPage)
    , m_photoManager(nullptr)
    , m_flowLayout(nullptr)
    , m_libraryItem(nullptr)
    , m_albumsItem(nullptr)
{
    ui->setupUi(this);
    setupUI();
    setupAlbumTree();
}

PhotoPage::~PhotoPage()
{
    clearPhotoGrid();
    delete ui;
}

void PhotoPage::setupUI()
{
    // 创建流式布局用于照片网格
    // 注意: UI 文件中 photoGridContainer 已经有 QVBoxLayout (photoGridLayout)
    // 这里将 FlowLayout 作为子布局追加到该 QVBoxLayout 中，避免 setLayout 失败导致 FlowLayout 未被安装
    {
        QLayout* containerLayout = ui->photoGridContainer->layout();
        if (auto vbox = qobject_cast<QVBoxLayout*>(containerLayout)) {
            m_flowLayout = new FlowLayout(nullptr, 8, 8, 8);
            vbox->addLayout(m_flowLayout);
            qDebug() << "[PhotoPage] setupUI: 使用已有的 QVBoxLayout 作为容器"
                     << "vbox=" << vbox << "flow=" << m_flowLayout;
        } else {
            // 兜底: 若无布局则直接设置 FlowLayout
            m_flowLayout = new FlowLayout(ui->photoGridContainer, 8, 8, 8);
            ui->photoGridContainer->setLayout(m_flowLayout);
            qDebug() << "[PhotoPage] setupUI: 直接设置 FlowLayout 为容器布局"
                     << "flow=" << m_flowLayout;
        }
    }
    
    // 连接信号
    connect(ui->refreshButton, &QPushButton::clicked, this, &PhotoPage::onRefreshClicked);
    connect(ui->exportButton, &QPushButton::clicked, this, &PhotoPage::onExportClicked);
    connect(ui->albumTree, &QTreeWidget::currentItemChanged, this, &PhotoPage::onAlbumSelectionChanged);
}

void PhotoPage::setupAlbumTree()
{
    ui->albumTree->clear();
    
    // 重置项指针
    m_libraryItem = nullptr;
    m_albumsItem = nullptr;
    
    // 图库 - 固定项，显示所有照片
    m_libraryItem = new QTreeWidgetItem(ui->albumTree);
    m_libraryItem->setText(0, "📷 图库");
    m_libraryItem->setData(0, Qt::UserRole, "library");
    
    // 我的相簿 - 动态加载的相册容器
    m_albumsItem = new QTreeWidgetItem(ui->albumTree);
    m_albumsItem->setText(0, "📁 我的相簿");
    m_albumsItem->setData(0, Qt::UserRole, "albums");
    m_albumsItem->setFlags(m_albumsItem->flags() | Qt::ItemIsAutoTristate);
    
    // 默认选中图库
    ui->albumTree->setCurrentItem(m_libraryItem);
    
    // 展开所有项
    ui->albumTree->expandAll();
}

void PhotoPage::setPhotoManager(PhotoManager *manager)
{
    if (m_photoManager) {
        disconnect(m_photoManager, nullptr, this, nullptr);
    }
    
    m_photoManager = manager;
    
    if (m_photoManager) {
        connect(m_photoManager, &PhotoManager::scanProgress, this, &PhotoPage::onScanProgress);
        connect(m_photoManager, &PhotoManager::errorOccurred, this, &PhotoPage::onPhotoError);
    }
}

void PhotoPage::setCurrentDevice(const QString &udid)
{
    m_currentUdid = udid;
    ui->statusLabel->setText("设备已连接，点击刷新按钮加载照片");
}

void PhotoPage::clearDevice()
{
    m_currentUdid.clear();
    m_currentAlbumPath.clear();
    clearPhotoGrid();
    updateStats(0, 0);
    ui->albumTitleLabel->setText("全部照片");
    ui->statusLabel->setText("请先连接设备以查看照片");
    
    // 清除相册树中的动态相册
    while (m_albumsItem->childCount() > 0) {
        delete m_albumsItem->takeChild(0);
    }
}

void PhotoPage::refreshPhotos()
{
    onRefreshClicked();
}

void PhotoPage::onRefreshClicked()
{
    if (m_currentUdid.isEmpty()) {
        QMessageBox::information(this, "提示", "请先连接设备");
        return;
    }
    
    if (!m_photoManager) {
        QMessageBox::warning(this, "错误", "照片管理器未初始化");
        return;
    }
    
    ui->statusLabel->setText("正在加载照片列表...");
    ui->refreshButton->setEnabled(false);
    QApplication::processEvents();
    
    // 连接到设备
    if (!m_photoManager->isConnected()) {
        if (!m_photoManager->connectToDevice(m_currentUdid)) {
            ui->statusLabel->setText(QString("连接失败: %1").arg(m_photoManager->lastError()));
            ui->refreshButton->setEnabled(true);
            return;
        }
    }
    
    // 获取相册列表
    QVector<AlbumInfo> albums = m_photoManager->getAlbums();
    updateAlbumTree(albums);
    
    // 获取当前相册的照片
    QVector<PhotoInfo> photos;
    if (m_currentAlbumPath.isEmpty()) {
        photos = m_photoManager->getAllPhotos();
    } else {
        photos = m_photoManager->getPhotos(m_currentAlbumPath);
    }
    
    if (photos.isEmpty() && !m_photoManager->lastError().isEmpty()) {
        ui->statusLabel->setText(QString("加载失败: %1").arg(m_photoManager->lastError()));
    } else {
        displayPhotos(photos);
        ui->statusLabel->setText("如要删除此目录内照片，请到设备上操作。");
    }
    
    ui->refreshButton->setEnabled(true);
}

void PhotoPage::onExportClicked()
{
    QVector<PhotoInfo> selected = getSelectedPhotos();
    
    if (selected.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要导出的照片");
        return;
    }
    
    QString dir = QFileDialog::getExistingDirectory(this, "选择导出目录");
    if (dir.isEmpty()) {
        return;
    }

    if (!m_photoManager || !m_photoManager->isConnected()) {
        QMessageBox::warning(this, "错误", "设备未连接");
        return;
    }

    // 创建进度对话框
    QProgressDialog progress("正在导出照片...", "取消", 0, selected.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0); // 立即显示
    progress.setValue(0);

    int successCount = 0;
    int failCount = 0;
    QString lastError;

    for (int i = 0; i < selected.size(); ++i) {
        if (progress.wasCanceled()) {
            break;
        }

        const PhotoInfo& photo = selected[i];
        progress.setLabelText(QString("正在导出 (%1/%2): %3").arg(i + 1).arg(selected.size()).arg(photo.name));
        
        // 读取照片数据
        QByteArray data = m_photoManager->readPhotoData(photo.path);
        if (data.isEmpty()) {
            failCount++;
            lastError = m_photoManager->lastError();
            qDebug() << "[PhotoPage] 导出失败(读取错误):" << photo.path << lastError;
        } else {
            // 写入本地文件
            QString targetPath = QDir(dir).filePath(photo.name);
            // 如果文件已存在，自动重命名: name_1.jpg, name_2.jpg
            if (QFile::exists(targetPath)) {
                QFileInfo fi(targetPath);
                int counter = 1;
                while (QFile::exists(targetPath)) {
                    targetPath = QDir(dir).filePath(QString("%1_%2.%3")
                        .arg(fi.baseName())
                        .arg(counter++)
                        .arg(fi.suffix()));
                }
            }

            QFile file(targetPath);
            if (file.open(QIODevice::WriteOnly)) {
                if (file.write(data) == data.size()) {
                    successCount++;
                } else {
                    failCount++;
                    lastError = "写入文件失败";
                    qDebug() << "[PhotoPage] 导出失败(写入错误):" << targetPath;
                }
                file.close();
            } else {
                failCount++;
                lastError = "无法创建目标文件";
                qDebug() << "[PhotoPage] 导出失败(创建文件错误):" << targetPath;
            }
        }

        progress.setValue(i + 1);
        QApplication::processEvents(); // 保持界面响应
    }

    progress.close();

    QString resultMsg = QString("导出完成\n成功: %1\n失败: %2").arg(successCount).arg(failCount);
    if (failCount > 0 && !lastError.isEmpty()) {
        resultMsg += QString("\n\n最后一次错误: %1").arg(lastError);
    }

    QMessageBox::information(this, "导出结果", resultMsg);
}

void PhotoPage::onAlbumSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous)
    
    if (!current) {
        qDebug() << "[PhotoPage] onAlbumSelectionChanged: current is null";
        return;
    }
    
    QString albumType = current->data(0, Qt::UserRole).toString();
    QString albumPath = current->data(0, Qt::UserRole + 1).toString();
    
    qDebug() << "[PhotoPage] onAlbumSelectionChanged:"
             << "text=" << current->text(0)
             << "type=" << albumType
             << "path=" << albumPath;
    
    // 更新标题
    ui->albumTitleLabel->setText(current->text(0));
    
    // 更新当前相册路径
    if (albumType == "library" || albumType == "albums") {
        m_currentAlbumPath.clear(); // 显示全部
        qDebug() << "[PhotoPage] -> 显示全部照片 (m_currentAlbumPath cleared)";
    } else if (albumType == "album") {
        m_currentAlbumPath = albumPath;
        qDebug() << "[PhotoPage] -> 显示相册:" << m_currentAlbumPath;
    } else {
        qDebug() << "[PhotoPage] -> 未知类型，不处理";
        return;
    }
    
    // 如果已连接设备，只刷新照片（不重新加载相册树）
    if (!m_currentUdid.isEmpty() && m_photoManager && m_photoManager->isConnected()) {
        qDebug() << "[PhotoPage] -> 调用 loadPhotosForCurrentAlbum()";
        loadPhotosForCurrentAlbum();
    } else {
        qDebug() << "[PhotoPage] -> 跳过加载: udid=" << m_currentUdid
                 << "manager=" << (m_photoManager ? "有" : "无")
                 << "connected=" << (m_photoManager ? m_photoManager->isConnected() : false);
    }
}

void PhotoPage::loadPhotosForCurrentAlbum()
{
    qDebug() << "[PhotoPage] loadPhotosForCurrentAlbum: albumPath=" << m_currentAlbumPath;
    
    if (!m_photoManager || !m_photoManager->isConnected()) {
        qDebug() << "[PhotoPage] loadPhotosForCurrentAlbum: manager无效或未连接";
        return;
    }
    
    ui->statusLabel->setText("正在加载照片列表...");
    QApplication::processEvents();
    
    // 获取当前相册的照片
    QVector<PhotoInfo> photos;
    if (m_currentAlbumPath.isEmpty()) {
        qDebug() << "[PhotoPage] 调用 getAllPhotos()";
        photos = m_photoManager->getAllPhotos();
    } else {
        qDebug() << "[PhotoPage] 调用 getPhotos(" << m_currentAlbumPath << ")";
        photos = m_photoManager->getPhotos(m_currentAlbumPath);
    }
    
    qDebug() << "[PhotoPage] 获取到照片数量:" << photos.size()
             << "lastError:" << m_photoManager->lastError();
    
    if (photos.isEmpty() && !m_photoManager->lastError().isEmpty()) {
        ui->statusLabel->setText(QString("加载失败: %1").arg(m_photoManager->lastError()));
    } else {
        displayPhotos(photos);
        ui->statusLabel->setText("如要删除此目录内照片，请到设备上操作。");
    }
}

void PhotoPage::onScanProgress(int current, int total)
{
    ui->statusLabel->setText(QString("正在扫描: %1 / %2").arg(current).arg(total));
    emit loadProgress(current, total);
    QApplication::processEvents();
}

void PhotoPage::onPhotoError(const QString &error)
{
    ui->statusLabel->setText(QString("错误: %1").arg(error));
    emit errorOccurred(error);
}

void PhotoPage::updateAlbumTree(const QVector<AlbumInfo> &albums)
{
    // 保存当前选中的相册路径，以便刷新后恢复选中
    QString currentPath = m_currentAlbumPath;
    QTreeWidgetItem *itemToSelect = nullptr;

    // 暂时屏蔽信号，防止在重建树的过程中触发不必要的 selectionChanged
    // 从而导致 m_currentAlbumPath 被错误重置
    const bool wasBlocked = ui->albumTree->blockSignals(true);

    // 清除现有相册
    while (m_albumsItem->childCount() > 0) {
        delete m_albumsItem->takeChild(0);
    }
    
    // 添加相册
    for (const AlbumInfo &album : albums) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_albumsItem);
        item->setText(0, album.name);
        item->setData(0, Qt::UserRole, "album");
        item->setData(0, Qt::UserRole + 1, album.path);
        
        // 如果有照片数量，显示在名称后
        if (album.photoCount > 0) {
            item->setText(0, QString("%1 (%2)").arg(album.name).arg(album.photoCount));
        }

        // 检查是否是之前选中的相册
        if (!currentPath.isEmpty() && album.path == currentPath) {
            itemToSelect = item;
        }
    }
    
    // 展开相簿
    m_albumsItem->setExpanded(true);

    // 恢复选中状态
    if (itemToSelect) {
        ui->albumTree->setCurrentItem(itemToSelect);
    }

    // 恢复信号
    ui->albumTree->blockSignals(wasBlocked);
}

void PhotoPage::displayPhotos(const QVector<PhotoInfo> &photos)
{
    qDebug() << "[PhotoPage] displayPhotos: 收到" << photos.size() << "张照片";
    
    clearPhotoGrid();
    
    qDebug() << "[PhotoPage] displayPhotos: flowLayout=" << m_flowLayout
             << "parent=" << (m_flowLayout ? m_flowLayout->parentWidget() : nullptr)
             << "container=" << ui->photoGridContainer
             << "container.layout=" << (ui->photoGridContainer ? ui->photoGridContainer->layout() : nullptr)
             << "container.visible=" << (ui->photoGridContainer ? ui->photoGridContainer->isVisible() : false)
             << "container.size=" << (ui->photoGridContainer ? ui->photoGridContainer->size() : QSize());
    
    int photoCount = 0;
    int videoCount = 0;
    
    for (const PhotoInfo &photo : photos) {
        PhotoThumbnail *thumbnail = new PhotoThumbnail(photo, ui->photoGridContainer);
        m_flowLayout->addWidget(thumbnail);
        thumbnail->show(); // 确保可见
        m_thumbnails.append(thumbnail);
        
        if (photo.isVideo) {
            videoCount++;
        } else {
            photoCount++;
        }
        
        // TODO: 异步加载缩略图
        // 目前使用占位符
    }
    
    // 激活容器布局，强制执行子布局几何更新
    if (ui->photoGridContainer->layout()) {
        ui->photoGridContainer->layout()->activate();
    }
    ui->photoGridContainer->updateGeometry();
    ui->photoGridContainer->update();
    
    qDebug() << "[PhotoPage] displayPhotos: 添加了" << m_thumbnails.size() << "个缩略图"
             << "layoutCount=" << m_flowLayout->count();
    
    // 检查第一个缩略图状态
    if (!m_thumbnails.isEmpty()) {
        PhotoThumbnail *first = m_thumbnails.first();
        qDebug() << "[PhotoPage] 第一个缩略图: parent=" << first->parentWidget()
                 << "visible=" << first->isVisible()
                 << "geometry=" << first->geometry();
    }
    
    updateStats(photoCount, videoCount);

    // 开始异步加载缩略图
    startThumbnailLoading();
}

void PhotoPage::clearPhotoGrid()
{
    // 清空待加载队列，防止访问已删除的对象
    m_pendingThumbnails.clear();
    m_isLoadingThumbnails = false;

    for (PhotoThumbnail *thumbnail : m_thumbnails) {
        m_flowLayout->removeWidget(thumbnail);
        thumbnail->deleteLater();
    }
    m_thumbnails.clear();
}

void PhotoPage::startThumbnailLoading()
{
    // 停止当前的加载（如果有）
    m_pendingThumbnails.clear();
    m_isLoadingThumbnails = false;
    
    // 将所有需要加载缩略图的项目加入队列
    // 暂时跳过视频文件，因为读取大文件会阻塞，且 QImage 无法解码视频
    for (PhotoThumbnail *thumbnail : m_thumbnails) {
        if (!thumbnail->photoInfo().isVideo) {
            m_pendingThumbnails.append(thumbnail);
        }
    }
    
    if (!m_pendingThumbnails.isEmpty()) {
        m_isLoadingThumbnails = true;
        // 使用 0ms 定时器在下一次事件循环开始加载，避免阻塞当前操作
        QTimer::singleShot(0, this, &PhotoPage::loadNextThumbnail);
    }
}

void PhotoPage::loadNextThumbnail()
{
    if (m_pendingThumbnails.isEmpty()) {
        m_isLoadingThumbnails = false;
        qDebug() << "[PhotoPage] 缩略图加载完成";
        return;
    }

    // 取出下一个要加载的缩略图
    // 注意：这里需要使用 QPointer 或者确保 clearPhotoGrid 时清空了队列
    // 我们已经在 clearPhotoGrid 中处理了清空，所以这里应该是安全的
    PhotoThumbnail *thumbnail = m_pendingThumbnails.takeFirst();
    
    // 双重检查：确保 thumbnail 仍然有效（虽然我们清空了队列，但在某些极端情况下为了安全）
    if (thumbnail && m_thumbnails.contains(thumbnail)) {
        QString path = thumbnail->photoInfo().path;
        QString ext = QFileInfo(path).suffix().toLower();
        
        // 读取照片数据
        // 注意：这是同步读取，对于大文件可能会有轻微卡顿
        QByteArray data = m_photoManager->readPhotoData(path);
        
        if (!data.isEmpty()) {
            QImage image;
            // 尝试从数据加载图片
            if (image.loadFromData(data)) {
                // 缩放图片以节省内存
                QPixmap pixmap = QPixmap::fromImage(image);
                thumbnail->setThumbnail(pixmap);
            } else {
                qDebug() << "[PhotoPage] 图片解码失败:" << path
                         << "格式:" << ext
                         << "数据大小:" << formatFileSize(data.size());
                
                if (ext == "heic" || ext == "heif") {
                    qDebug() << "[PhotoPage] 提示: Qt 可能缺少 HEIC/HEIF 图像格式插件";
                }
            }
        } else {
             qDebug() << "[PhotoPage] 读取数据失败(空):" << path << "错误:" << m_photoManager->lastError();
        }
    }
    
    // 继续加载下一个
    // 使用 1ms 延迟给 UI 线程喘息的机会，保持界面响应
    if (m_isLoadingThumbnails) {
        QTimer::singleShot(1, this, &PhotoPage::loadNextThumbnail);
    }
}

void PhotoPage::updateStats(int photoCount, int videoCount)
{
    ui->photoCountLabel->setText(
        QString("%1 张照片，%2 个视频").arg(photoCount).arg(videoCount));
}

QVector<PhotoInfo> PhotoPage::getSelectedPhotos() const
{
    QVector<PhotoInfo> selected;
    for (PhotoThumbnail *thumbnail : m_thumbnails) {
        if (thumbnail->isSelected()) {
            selected.append(thumbnail->photoInfo());
        }
    }
    return selected;
}

QString PhotoPage::formatFileSize(qint64 size) const
{
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}