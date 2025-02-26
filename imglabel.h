#ifndef IMGLABEL_H
#define IMGLABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QDebug>
class ImgLabel : public QLabel
{
public:
    ImgLabel(QWidget *parent=nullptr);
    ~ImgLabel();
    void setPixmap(const QPixmap &pixmap);
    void clear();
    void resetPos(QPoint p);
protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
private:  
    void clampDrawPosition();   // 边界检查函数
    void updateCachedPixmap();  // 初始化缓存
    double m_scaleValue;        // 图片缩放倍数
    QPoint m_drawPoint;         // 绘图起点
    QPoint m_mousePoint;        // 鼠标当前位置点
    QRect m_rectPixmap;         // 被绘图片的矩形范围
    bool m_isMousePress;        // 鼠标是否按下
    bool m_isCtrlPress;         // Ctrl键是否被按下
    bool m_isFirstTime;         // 是否为第一次
    const double SCALE_MAX_VALUE = 10.0;//最大放大到原来的10倍
    const double SCALE_MIN_VALUE = 0.05;//最小缩小到原来的0.05倍

    QPixmap m_cachedScaledPixmap;       // 新增缓存成员变量
    double m_lastCachedScale = -1.0;    // 记录上次缓存的缩放比例
};

#endif // IMGLABEL_H
