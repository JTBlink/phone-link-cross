/**
 * @file photopage.h
 * @brief 照片页面组件头文件
 *
 * 本文件定义了照片浏览和管理页面的UI组件类。
 * 提供相册浏览、照片网格显示、导出等功能。
 */

#ifndef PHOTOPAGE_H
#define PHOTOPAGE_H

#include <QWidget>
#include <QVector>
#include <QMap>

#include "core/photo/photomanager.h"

// 前向声明
class QTreeWidgetItem;
class FlowLayout;

QT_BEGIN_NAMESPACE
namespace Ui {
class PhotoPage;
}
QT_END_NAMESPACE

/**
 * @brief 照片缩略图组件类
 *
 * 用于显示单张照片的缩略图，支持选择状态和悬停效果。
 */
class PhotoThumbnail : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoThumbnail(const PhotoInfo &info, QWidget *parent = nullptr);
    
    /**
     * @brief 获取照片信息
     * @return 照片信息结构
     */
    const PhotoInfo& photoInfo() const { return m_photoInfo; }
    
    /**
     * @brief 设置选中状态
     * @param selected 是否选中
     */
    void setSelected(bool selected);
    
    /**
     * @brief 获取选中状态
     * @return 是否选中
     */
    bool isSelected() const { return m_selected; }
    
    /**
     * @brief 设置缩略图
     * @param pixmap 缩略图图像
     */
    void setThumbnail(const QPixmap &pixmap);

signals:
    /**
     * @brief 点击信号
     */
    void clicked();
    
    /**
     * @brief 双击信号
     */
    void doubleClicked();
    
    /**
     * @brief 选择状态改变信号
     * @param selected 新的选择状态
     */
    void selectionChanged(bool selected);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    PhotoInfo m_photoInfo;     ///< 照片信息
    QPixmap m_thumbnail;       ///< 缩略图
    bool m_selected;           ///< 是否选中
    bool m_hovered;            ///< 是否悬停
};

/**
 * @brief 照片页面类
 *
 * 提供完整的照片浏览和管理界面，包括：
 * - 左侧相册树形导航
 * - 右侧照片网格显示
 * - 照片导出功能
 * - 照片信息显示
 */
class PhotoPage : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoPage(QWidget *parent = nullptr);
    ~PhotoPage();
    
    /**
     * @brief 设置照片管理器
     * @param manager 照片管理器指针
     */
    void setPhotoManager(PhotoManager *manager);
    
    /**
     * @brief 设置当前设备UDID
     * @param udid 设备UDID
     */
    void setCurrentDevice(const QString &udid);
    
    /**
     * @brief 清空当前设备
     */
    void clearDevice();
    
    /**
     * @brief 刷新照片列表
     */
    void refreshPhotos();

signals:
    /**
     * @brief 加载进度信号
     * @param current 当前进度
     * @param total 总数
     */
    void loadProgress(int current, int total);
    
    /**
     * @brief 发生错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

private slots:
    /**
     * @brief 刷新按钮点击槽
     */
    void onRefreshClicked();
    
    /**
     * @brief 导出按钮点击槽
     */
    void onExportClicked();
    
    /**
     * @brief 相册选择改变槽
     * @param current 当前选中项
     * @param previous 之前选中项
     */
    void onAlbumSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    
    /**
     * @brief 照片扫描进度槽
     * @param current 当前进度
     * @param total 总数
     */
    void onScanProgress(int current, int total);
    
    /**
     * @brief 照片错误槽
     * @param error 错误信息
     */
    void onPhotoError(const QString &error);

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();
    
    /**
     * @brief 初始化相册树
     */
    void setupAlbumTree();
    
    /**
     * @brief 更新相册树
     * @param albums 相册列表
     */
    void updateAlbumTree(const QVector<AlbumInfo> &albums);
    
    /**
     * @brief 显示照片列表
     * @param photos 照片列表
     */
    void displayPhotos(const QVector<PhotoInfo> &photos);
    
    /**
     * @brief 清空照片网格
     */
    void clearPhotoGrid();
    
    /**
     * @brief 加载当前相册的照片（不重新加载相册树）
     */
    void loadPhotosForCurrentAlbum();

    /**
     * @brief 启动缩略图加载队列
     * 将当前显示的缩略图加入待加载队列并开始逐个加载，避免一次性阻塞UI
     */
    void startThumbnailLoading();

    /**
     * @brief 加载队列中的下一个缩略图
     * 读取设备上的图片数据并设置到对应的 PhotoThumbnail
     */
    void loadNextThumbnail();
    
    /**
     * @brief 更新统计信息
     * @param photoCount 照片数量
     * @param videoCount 视频数量
     */
    void updateStats(int photoCount, int videoCount);
    
    /**
     * @brief 获取选中的照片
     * @return 选中的照片列表
     */
    QVector<PhotoInfo> getSelectedPhotos() const;
    
    /**
     * @brief 格式化文件大小
     * @param size 字节大小
     * @return 格式化后的字符串
     */
    QString formatFileSize(qint64 size) const;

    Ui::PhotoPage *ui;                      ///< UI指针
    PhotoManager *m_photoManager;           ///< 照片管理器
    QString m_currentUdid;                  ///< 当前设备UDID
    QString m_currentAlbumPath;             ///< 当前相册路径
    
    FlowLayout *m_flowLayout;               ///< 照片网格布局
    QVector<PhotoThumbnail*> m_thumbnails;  ///< 缩略图列表

    // 缩略图加载队列与状态
    QVector<PhotoThumbnail*> m_pendingThumbnails; ///< 待加载缩略图队列
    bool m_isLoadingThumbnails = false;            ///< 是否正在加载缩略图
    
    // 相册树项
    QTreeWidgetItem *m_libraryItem;         ///< 图库（显示所有照片）
    QTreeWidgetItem *m_albumsItem;          ///< 我的相簿（动态加载的相册容器）
};

#endif // PHOTOPAGE_H