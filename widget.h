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

    QString version = "v1.0.3";

    Widget(QWidget *parent = nullptr);
    ~Widget();
    QSize parseImageSize(const QString& fileName);
    void getCVMatGRBG();
    void getCVMatGaussian();
    void getQImage();
    void setCircleInfo();
    void cvLine(cv::Point pt1, cv::Point pt2, double &distance);
    void sortCirclePoint();
    void init();
    void changeGain(double contrast, int brightness); // 改变亮度和对比度
    QImage cvMatToQImage(const cv::Mat &mat);
    cv::Mat qImageToCVMat(const QImage &qImage);
private slots:
    void on_pushButton_chooseImg_clicked();

    void on_pushButton_opencv_clicked();

    void on_pushButton_labelImg_gray8_clicked();

    void on_pushButton_clear_clicked();

    void on_pushButton_opencv_gray_clicked();

    void on_pushButton_circle_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_canny_clicked();

    void on_pushButton_gray_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_threshold_clicked();

    void on_spinBox_brightness_valueChanged(int arg1);

    void on_horizontalSlider_brightness_valueChanged(int value);

    void on_doubleSpinBox_contrast_valueChanged(double arg1);

    void on_horizontalSlider_contrast_valueChanged(int value);

private:
    Ui::Widget *ui;
    QString filePath;
    QString savePath;
    int width;
    int height;
    cv::Mat img_mat_root;
    cv::Mat img_mat_gray;
    cv::Mat img_mat_gaussian;
    cv::Mat img_mat_circle;
    cv::Mat img_mat_canny;
    QImage qImg;
    std::vector<quint16> rawData;
    std::vector<cv::Vec3f> circles;
    QString imgType;
    int threshType = 0;
};
#endif // WIDGET_H
