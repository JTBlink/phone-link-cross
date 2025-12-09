/**
 * @file flowlayout.h
 * @brief 流式布局类头文件
 *
 * 实现一个自动换行的流式布局，用于照片网格显示。
 */

#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QStyle>

/**
 * @brief 流式布局类
 *
 * 自动根据容器宽度排列子项，超出宽度时自动换行。
 * 适用于照片网格等需要自适应排列的场景。
 */
class FlowLayout : public QLayout
{
    Q_OBJECT

public:
    explicit FlowLayout(QWidget *parent = nullptr, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout();

    /**
     * @brief 添加项目
     */
    void addItem(QLayoutItem *item) override;
    
    /**
     * @brief 获取水平间距
     */
    int horizontalSpacing() const;
    
    /**
     * @brief 获取垂直间距
     */
    int verticalSpacing() const;
    
    /**
     * @brief 设置水平间距
     */
    void setHorizontalSpacing(int spacing);
    
    /**
     * @brief 设置垂直间距
     */
    void setVerticalSpacing(int spacing);
    
    /**
     * @brief 扩展方向
     */
    Qt::Orientations expandingDirections() const override;
    
    /**
     * @brief 是否有高度取决于宽度
     */
    bool hasHeightForWidth() const override;
    
    /**
     * @brief 根据宽度计算高度
     */
    int heightForWidth(int width) const override;
    
    /**
     * @brief 项目数量
     */
    int count() const override;
    
    /**
     * @brief 获取指定索引的项目
     */
    QLayoutItem *itemAt(int index) const override;
    
    /**
     * @brief 最小尺寸
     */
    QSize minimumSize() const override;
    
    /**
     * @brief 设置几何形状
     */
    void setGeometry(const QRect &rect) override;
    
    /**
     * @brief 尺寸提示
     */
    QSize sizeHint() const override;
    
    /**
     * @brief 移除指定索引的项目
     */
    QLayoutItem *takeAt(int index) override;

private:
    /**
     * @brief 执行布局计算
     * @param rect 布局区域
     * @param testOnly 是否仅测试（不实际布局）
     * @return 布局高度
     */
    int doLayout(const QRect &rect, bool testOnly) const;
    
    /**
     * @brief 获取智能间距
     * @param pm 样式像素度量类型
     * @return 间距值
     */
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_itemList;  ///< 布局项目列表
    int m_hSpace;                      ///< 水平间距
    int m_vSpace;                      ///< 垂直间距
};

#endif // FLOWLAYOUT_H