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

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    double m_scaleValue;      // 图片缩放倍数
    QPointF m_drawPoint;      // 绘图起点
    QPointF m_mousePoint;     // 鼠标当前位置点
    QRect m_rectPixmap;       // 被绘图片的矩形范围
    bool m_isMousePress;      // 鼠标是否按下

    const double SCALE_MAX_VALUE = 10.0;//最大放大到原来的10倍
    const double SCALE_MIN_VALUE = 0.05;//最小缩小到原来的0.5倍
};

#endif // IMGLABEL_H
