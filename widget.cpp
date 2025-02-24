#include "widget.h"
#include "ui_widget.h"
#include <QFileInfo>
#include <QFile>
#include <fstream>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setLayout(ui->mainLayout);
    ui->pushButton_opencv->setDisabled(true);
    ui->pushButton_opencv_gray->setDisabled(true);
    ui->pushButton_labelImg_gray16->setDisabled(true);
    ui->pushButton_labelImg_gray8->setDisabled(true);
    ui->lineEdit->setDisabled(true);
}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_pushButton_chooseImg_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
                                            "请选择raw图",
                                            "",
                                            "Images (*.raw);");
    if(path.isEmpty()){
        qDebug() << "用户取消选择";
        return;
    }
    filePath = path;
    ui->lineEdit->setText(filePath);
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    qDebug() << fileName;
    QSize size = parseImageSize(fileName);
    if(size.isEmpty()){
        qDebug() << "raw file is not named with size!";
        return;
    }
    qDebug() << "width:" << size.width() << ", height:" << size.height();
    width = size.width();
    height = size.height();

    size_t dataSize = width * height * sizeof(quint16);;

    std::ifstream ifs(filePath.toStdString());
    if(!ifs.is_open()){
        qDebug() << filePath << " cant open";
        return;
    }
    ifs.seekg(0, std::ios::end);
    size_t actual_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    // 匹配文件大小 防止宽高错误
    if (actual_size != dataSize) {
        qDebug() << "文件大小不匹配!";
        return;
    }
    // 读取数据到缓冲区
    rawData.resize(width * height);
    ifs.read(reinterpret_cast<char*>(rawData.data()), actual_size);
    ifs.close();

    ui->pushButton_opencv->setDisabled(false);
    ui->pushButton_opencv_gray->setDisabled(false);
    ui->pushButton_labelImg_gray16->setDisabled(false);
    ui->pushButton_labelImg_gray8->setDisabled(false);
    // 获取cv的mat包括灰度图与GRBG图
    getCVMatGRBG();
    getCVMatGray();
    // 获取qImg 8位与16位
    getQImage();
    getQImageGray16();
    ui->label->clear();
    ui->label->setPixmap(QPixmap::fromImage(qImg));
}



QSize Widget::parseImageSize(const QString &fileName)
{
    QRegularExpression re("(\\d+)X(\\d+)");
    QRegularExpressionMatch match = re.match(fileName);
    if (match.hasMatch()) {
        return QSize(match.captured(1).toInt(),
                     match.captured(2).toInt());
    }
    return QSize(0, 0); // 无效尺寸
}

void Widget::getCVMatGRBG()
{
    // 创建OpenCV矩阵（16位单通道）
    cv::Mat raw_image(height, width, CV_16UC1, rawData.data());

    // 检查数据范围
    double min_val, max_val;
    cv::minMaxLoc(raw_image, &min_val, &max_val);
    qDebug() << "实际数据范围: " << min_val << " ~ " << max_val;

    // 3. 转换10位数据（假设高位有效）
    cv::Mat converted_image;
    raw_image.convertTo(converted_image, CV_16UC1, 64.0); // 10bit -> 16bit (1023 * 64=65520)
    // 4. Bayer去马赛克（GRBG转BGR）
    cv::Mat color_image;
    cv::cvtColor(converted_image, color_image, cv::COLOR_BayerGR2BGR);
    // 5. 转换为8位显示
    cv::Mat display_image;
    double scale_factor = 255.0 / 65535.0; // 16bit转8bit
    color_image.convertTo(display_image, CV_8UC3, scale_factor);
    img_mat_grbg = display_image;
}

void Widget::getCVMatGray()
{
    //归一化到0-255范围以便显示 OpenCV默认显示8位
    cv::Mat grayImage(height, width, CV_16UC1);

    for(int j = 0; j < height; j++){
        for(int i = 0; i < width; i++){
            // unpacked raw10 右移两位
            quint16 pixel = rawData[i + j * width];
            // 为每个像素赋值
            grayImage.at<quint16>(j, i) = pixel;
        }
    }
    // 归一化到0-255范围以便显示（OpenCV默认显示8位）
    cv::Mat displayImage;
    double scale = 255.0 / 1023.0;  // 10位范围是0~1023
    /* convertTo()是 OpenCV 中用于 数据类型转换和数值范围缩放 的关键操作。
     * 它的作用是将原始的高位深灰度图像（例如 10 位或 16 位）转换为 8 位图像，以便用 imshow 正确显示。
    */
    grayImage.convertTo(displayImage, CV_8UC1, scale);
    img_mat_gray = displayImage;
}

void Widget::getQImage()
{
    QImage image(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // QImage::Format_Grayscale8的范围是0~255
            // 需要对0~1024像素值进行缩放映射到0~255对应范围中
            quint8 pixel = rawData[y * width + x] >> 2;
            // 设置像素值
            // 使用 qRgb 函数将灰度值转换为 QRgb 类型
            QRgb gray = qRgb(pixel, pixel, pixel);
            image.setPixel(x, y, gray);
        }
    }
    // 显示图片
    qImg = image;
}

void Widget::getQImageGray16()
{
    QImage image(width, height, QImage::Format_Grayscale16);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // QImage::Format_Grayscale16的范围是0~65535
            // 需要对像素值进行缩放映射到对应范围中
            quint16 pixel = rawData[y * width + x] << 6;
            // 设置像素值
            // 使用 qRgb 函数将灰度值转换为 QRgb 类型
            QRgb gray = qRgb(pixel, pixel, pixel);
            image.setPixel(x, y, gray);
        }
    }
    qImg_gray16 = image;
}

void Widget::on_pushButton_opencv_clicked()
{
    // 显示图片
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_grbg)));
    // 显示图像
    //cv::namedWindow("RAW Image", cv::WINDOW_NORMAL);
    //cv::imshow("RAW Image", img_mat_grbg);
}


void Widget::on_pushButton_labelImg_gray16_clicked()
{

    // 显示图片
    ui->label->setPixmap(QPixmap::fromImage(qImg_gray16));
}


void Widget::on_pushButton_labelImg_gray8_clicked()
{
    // 显示图片
    ui->label->setPixmap(QPixmap::fromImage(qImg));
}


void Widget::on_pushButton_clear_clicked()
{
    ui->label->clear();
}


void Widget::on_pushButton_opencv_gray_clicked()
{

    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_gray)));
    //cv::namedWindow("RAW gray Image", cv::WINDOW_NORMAL);
    //cv::imshow("RAW gray Image", img_mat_gray);
}

QImage Widget::cvMatToQImage(const cv::Mat &mat){
    if (mat.empty()) {
        qDebug() << "error : mat is empty!";
        return QImage();
    }

    // 处理灰度图（8UC1）
    if(mat.type() == CV_8UC1){
        return QImage(
                   mat.data,            // 数据指针
                   mat.cols,            // 宽度
                   mat.rows,            // 高度
                   mat.step,            // 每行字节数
                   QImage::Format_Grayscale8 // 8位灰度格式
                   ).copy(); // 深拷贝避免悬空指针
    }
    // 处理BGR彩色图
    else if (mat.type() == CV_8UC3) {
        // 将BGR转换为RGB
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);

        return QImage(
                   rgbMat.data,         // 数据指针
                   rgbMat.cols,         // 宽度
                   rgbMat.rows,         // 高度
                   rgbMat.step,         // 每行字节数
                   QImage::Format_RGB888 // RGB888格式
                   ).copy(); // 深拷贝
    }
    // 不支持的类型返回空图像
    else {
        qWarning("Unsupported Mat type: must be CV_8UC1 or CV_8UC3");
        return QImage();
    }
}
