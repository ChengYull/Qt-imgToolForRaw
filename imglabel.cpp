#include "imglabel.h"

ImgLabel::ImgLabel(QWidget *parent):QLabel(parent)
    ,m_scaleValue(0.5)
    ,m_mousePoint(0,0)
    ,m_drawPoint(0,0)
    ,m_rectPixmap(0,0,0,0)
    ,m_isMousePress(false)
{
    setFixedSize(600, 600); // 设置固定大小
}

void ImgLabel::paintEvent(QPaintEvent *)
{

    if (!this->pixmap() || this->pixmap()->isNull()) {
        return; // 添加空指针检查
    }
    QPainter painter(this);
    QPixmap originalPixmap = *this->pixmap();

    double scaledWidth = originalPixmap.width() * m_scaleValue;
    double scaledHeight = originalPixmap.height() * m_scaleValue;
    QPixmap scaledPixmap = originalPixmap.scaled(scaledWidth, scaledHeight,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation);

    // 计算绘制起点（居中或可拖动）
    m_rectPixmap=QRect(m_drawPoint.x(), m_drawPoint.y(), scaledWidth, scaledHeight);  // 图片区域
    // 绘制图片
    painter.drawPixmap(m_rectPixmap, scaledPixmap);
}

void ImgLabel::mouseMoveEvent(QMouseEvent *event)
{
    if(m_isMousePress)
    {
        int x = event->pos().x()-m_mousePoint.x();
        int y = event->pos().y()-m_mousePoint.y();
        m_mousePoint = event->pos();
        m_drawPoint = QPointF(m_drawPoint.x() + x, m_drawPoint.y() + y);
        update();
    }
}

void ImgLabel::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        m_isMousePress=true;
        m_mousePoint=event->pos();
    }
}

void ImgLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        m_drawPoint=QPointF(0,0);
        m_scaleValue=1.0;
        update();
    }
    if(event->button() == Qt::LeftButton) m_isMousePress=false;
}

void ImgLabel::wheelEvent(QWheelEvent *event)
{
    int flag = event->angleDelta().y();
    double oldScale = m_scaleValue;
    m_scaleValue *= (flag > 0) ? 1.1 : 0.9;

    // 限制缩放范围
    m_scaleValue = qBound(SCALE_MIN_VALUE, m_scaleValue, SCALE_MAX_VALUE);
    QPointF pos = event->position();
    if (m_rectPixmap.contains(pos.toPoint())) {
        // 计算缩放中心点
        double ratio = m_scaleValue / oldScale;
        double newX = pos.x() - (pos.x() - m_drawPoint.x()) * ratio;
        double newY = pos.y() - (pos.y() - m_drawPoint.y()) * ratio;
        m_drawPoint = QPointF(newX, newY);
    } else {
        // 以图片中心缩放
        QPointF oldCenter = m_rectPixmap.center();
        double newWidth = this->width() * m_scaleValue;
        double newHeight = this->height() * m_scaleValue;
        m_drawPoint = QPointF(oldCenter.x() - newWidth/2, oldCenter.y() - newHeight/2);
    }
    update();
}

void ImgLabel::resizeEvent(QResizeEvent *event)
{
    m_drawPoint=QPointF(0,0);
    m_scaleValue=1.0;
    update();
}
