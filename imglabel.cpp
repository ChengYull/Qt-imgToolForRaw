#include "imglabel.h"

ImgLabel::ImgLabel(QWidget *parent):QLabel(parent)
    ,m_scaleValue(0.5)
    ,m_mousePoint(0,0)
    ,m_drawPoint(0,0)
    ,m_rectPixmap(0,0,0,0)
    ,m_isMousePress(false)
{
    setFixedSize(600, 600); // 设置固定大小
    setFocusPolicy(Qt::FocusPolicy::NoFocus);
}

ImgLabel::~ImgLabel()
{
    setFocusPolicy(Qt::FocusPolicy::NoFocus);
}

void ImgLabel::setPixmap(const QPixmap &pixmap)
{
    QLabel::setPixmap(pixmap);

    // 初始化缓存
    if (!pixmap.isNull()) {
        if(m_isFirstTime){
            m_scaleValue = 1.0; // 重置缩放比例
            m_isFirstTime = false;
        }
        m_lastCachedScale = -1.0; // 强制更新缓存
        updateCachedPixmap(); // 主动生成缓存
        clampDrawPosition(); // 确保绘制位置合法
    }
    update();
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

void ImgLabel::paintEvent(QPaintEvent *)
{
    if (m_cachedScaledPixmap.isNull()) return;

    QPainter painter(this);
    painter.drawPixmap(m_rectPixmap, m_cachedScaledPixmap);
}

void ImgLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->pixmap() || this->pixmap()->isNull()) return;
    if(m_isMousePress)
    {
        QPoint delta = event->pos() - m_mousePoint;
        m_mousePoint = event->pos();
        m_drawPoint += delta;
        // 更新绘制区域
        m_rectPixmap = QRect(
            m_drawPoint,
            m_cachedScaledPixmap.size()
            );
        clampDrawPosition(); // 新增边界检查
        update();
    }
}

void ImgLabel::mousePressEvent(QMouseEvent *event)
{
    if (!this->pixmap() || this->pixmap()->isNull()) return;
    if(event->button()==Qt::LeftButton)
    {
        m_isMousePress=true;
        m_mousePoint=event->pos();
    }
}

void ImgLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (!this->pixmap() || this->pixmap()->isNull()) return;
    if(event->button() == Qt::RightButton)
    {
        m_drawPoint = QPoint(0,0);
        m_scaleValue = 1.0;
        update();
    }
    if(event->button() == Qt::LeftButton) m_isMousePress=false;
}

void ImgLabel::wheelEvent(QWheelEvent *event)
{

    if (!this->pixmap() || this->pixmap()->isNull()) return;
    int flag = event->angleDelta().y();
    double oldScale = m_scaleValue;
    if(m_isCtrlPress)
        m_scaleValue *= (flag > 0) ? 2 : 0.5;
    else
        m_scaleValue *= (flag > 0) ? 1.2 : 0.8;
    // 限制缩放范围
    m_scaleValue = qBound(SCALE_MIN_VALUE, m_scaleValue, SCALE_MAX_VALUE);
    QPoint pos = event->position().toPoint();
    if (m_rectPixmap.contains(pos)) {
        // 计算缩放中心点
        double ratio = m_scaleValue / oldScale;
        double newX = pos.x() - (pos.x() - m_drawPoint.x()) * ratio;
        double newY = pos.y() - (pos.y() - m_drawPoint.y()) * ratio;
        m_drawPoint = QPoint(newX, newY);
    } else {
        // 以图片中心缩放
        QPoint oldCenter = m_rectPixmap.center();
        double newWidth = this->pixmap()->width() * m_scaleValue;
        double newHeight = this->pixmap()->height() * m_scaleValue;
        m_drawPoint = QPoint(oldCenter.x() - newWidth/2, oldCenter.y() - newHeight/2);
    }

    // 缩放比例变化时更新缓存
    if (m_scaleValue != m_lastCachedScale)
        updateCachedPixmap();
    clampDrawPosition();
    update();
}

void ImgLabel::resizeEvent(QResizeEvent *event)
{
    m_drawPoint = QPoint(0,0);
    m_scaleValue=1.0;
    update();
}

void ImgLabel::keyPressEvent(QKeyEvent *event)
{
    if (!this->pixmap() || this->pixmap()->isNull()) return;
    int u_step = 10;
    int d_step = -10;
    if(m_scaleValue > 1)
        std::swap(u_step, d_step);
    if((event->key() == Qt::Key_W || event->key() == Qt::Key_Up) && !this->pixmap()->isNull()){
        m_drawPoint.setY(m_drawPoint.y() + d_step);
        clampDrawPosition();
        // 更新绘制区域
        m_rectPixmap = QRect(m_drawPoint,m_cachedScaledPixmap.size());
        update();
    }else if(event->key() == Qt::Key_S || event->key() == Qt::Key_Down){
        m_drawPoint.setY(m_drawPoint.y() + u_step);
        clampDrawPosition();
        // 更新绘制区域
        m_rectPixmap = QRect(m_drawPoint,m_cachedScaledPixmap.size());
        update();
    }else if(event->key() == Qt::Key_A || event->key() == Qt::Key_Left){
        m_drawPoint.setX(m_drawPoint.x() + d_step);
        clampDrawPosition();
        // 更新绘制区域
        m_rectPixmap = QRect(m_drawPoint,m_cachedScaledPixmap.size());
        update();
    }else if(event->key() == Qt::Key_D || event->key() == Qt::Key_Right){
        m_drawPoint.setX(m_drawPoint.x() + u_step);
        clampDrawPosition();
        // 更新绘制区域
        m_rectPixmap = QRect(m_drawPoint,m_cachedScaledPixmap.size());
        update();
    }

    if(event->key() == Qt::Key_Control)
        m_isCtrlPress = true;



    return QLabel::keyPressEvent(event);
}

void ImgLabel::keyReleaseEvent(QKeyEvent *event)
{
    if (!this->pixmap() || this->pixmap()->isNull()) return;

    if(event->key() == Qt::Key_Control)
        m_isCtrlPress = false;


}

void ImgLabel::clear()
{
    this->m_cachedScaledPixmap = QPixmap();
    QLabel::clear();
}

void ImgLabel::resetPos(QPoint p)
{
    if (!this->pixmap() || this->pixmap()->isNull()) return;
    clampDrawPosition();
    // 更新绘制区域
    m_rectPixmap = QRect(p,m_cachedScaledPixmap.size());
    update();
}

void ImgLabel::clampDrawPosition() {
    QSize labelSize = size();
    QSize pixmapSize(
        this->pixmap()->width() * m_scaleValue,
        this->pixmap()->height() * m_scaleValue
        );

    // X 轴边界检查
    int minX = labelSize.width() - pixmapSize.width();
    int maxX = 0;
    if (pixmapSize.width() < labelSize.width()) {
        // 图片宽度小于 Label，允许自由拖动，但确保图片不超出边界
        minX = 0;
        maxX = labelSize.width() - pixmapSize.width();
    }
    m_drawPoint.setX(qBound(minX, (int)m_drawPoint.x(), maxX));

    // Y 轴边界检查（逻辑同 X 轴）
    int minY = labelSize.height() - pixmapSize.height();
    int maxY = 0;
    if (pixmapSize.height() < labelSize.height()) {
        minY = 0;
        maxY = labelSize.height() - pixmapSize.height();
    }
    m_drawPoint.setY(qBound(minY, (int)m_drawPoint.y(), maxY));
}

// 新增函数：更新缓存 -- 解决放大后拖动卡顿
void ImgLabel::updateCachedPixmap() {
    if (!this->pixmap() || this->pixmap()->isNull()) return;

    QPixmap originalPixmap = *this->pixmap();
    QSize scaledSize = originalPixmap.size() * m_scaleValue;
    m_cachedScaledPixmap = originalPixmap.scaled(
        scaledSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );
    m_lastCachedScale = m_scaleValue;

    // 更新绘制区域
    m_rectPixmap = QRect(
        m_drawPoint,
        m_cachedScaledPixmap.size()
        );
}
