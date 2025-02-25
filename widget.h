#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <QRegularExpression>
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    QSize parseImageSize(const QString& fileName);
    void getCVMatGRBG();
    void getCVMatGray();
    void getQImage();
    void getQImageGray16();
    void setCircleInfo();
    void cvLine(cv::Point pt1, cv::Point pt2, double &distance);
    void sortCirclePoint();
    QImage cvMatToQImage(const cv::Mat &mat);
private slots:
    void on_pushButton_chooseImg_clicked();

    void on_pushButton_opencv_clicked();

    void on_pushButton_labelImg_gray16_clicked();

    void on_pushButton_labelImg_gray8_clicked();

    void on_pushButton_clear_clicked();

    void on_pushButton_opencv_gray_clicked();

    void on_pushButton_circle_clicked();

    void on_pushButton_reset_clicked();

private:
    Ui::Widget *ui;
    QString filePath;
    int width;
    int height;
    cv::Mat img_mat_grbg;
    cv::Mat img_mat_gray;
    cv::Mat img_mat_circle;
    QImage qImg;
    QImage qImg_gray16;
    std::vector<quint16> rawData;
    std::vector<cv::Vec3f> circles;
};
#endif // WIDGET_H
