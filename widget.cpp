#include "widget.h"
#include "ui_widget.h"
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setLayout(ui->mainLayout);

    QString title = "图片工具" + version;
    this->setWindowTitle(title);

    ui->pushButton_opencv->setDisabled(true);
    ui->pushButton_opencv_gray->setDisabled(true);
    ui->pushButton_circle->setDisabled(true);
    ui->pushButton_canny->setDisabled(true);
    ui->pushButton_gray->setDisabled(true);
    ui->pushButton_save->setDisabled(true);
    ui->pushButton_labelImg_gray8->setDisabled(true);
    ui->pushButton_threshold->setDisabled(true);
    ui->lineEdit->setDisabled(true);
    ui->lineEdit_width->setDisabled(true);
    ui->lineEdit_height->setDisabled(true);

    ui->spinBox_brightness->setDisabled(true);
    ui->doubleSpinBox_contrast->setDisabled(true);
    ui->horizontalSlider_brightness->setDisabled(true);
    ui->horizontalSlider_contrast->setDisabled(true);
    ui->doubleSpinBox_contrast->setValue(1.0);
    ui->doubleSpinBox_contrast->setSingleStep(0.01);
    ui->horizontalSlider_contrast->setValue(10);
    ui->horizontalSlider_contrast->setMaximum(20);
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
                                            "Images (*.raw;*.png;*.jpg);");
    if(path.isEmpty()){
        qDebug() << "用户取消选择";
        return;
    }
    // 重选图片后执行初始化操作
    init();
    filePath = path;
    ui->lineEdit->setText(filePath);
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    imgType = fileInfo.suffix();
    qDebug() << fileName << fileInfo.suffix();
    if("raw" == imgType){
        QSize size = parseImageSize(fileName);
        if(size.isEmpty()){
            QMessageBox::warning(nullptr, "文件大小不匹配", "图片尺寸与文件大小不匹配");
            qDebug() << "raw file is not named with size!";
            return;
        }
        qDebug() << "width:" << size.width() << ", height:" << size.height();
        width = size.width();
        height = size.height();
        // 设置界面显示信息
        ui->lineEdit_width->setText(QString::number(width));
        ui->lineEdit_height->setText(QString::number(height));
        size_t dataSize = width * height * sizeof(quint16);

        // 使用QFile支持中文路径
        QFile file(filePath);
        if(!file.open(QIODevice::ReadOnly)){
            QMessageBox::warning(nullptr, "打开失败", "文件打开失败");
            qDebug() << filePath << " cant open";
            return;
        }
        size_t actual_size = file.size();
        if (actual_size != dataSize) {
            qDebug() << "文件大小不匹配!";
            return;
        }
        rawData.clear();
        while(!file.atEnd()){
            char buffer[2];
            qint64 byteRead = file.read(buffer, sizeof(buffer));
            if(byteRead == 2){
                // 组合为两个字节 LH
                quint16 val = static_cast<quint16>(static_cast<quint8>(buffer[0])) |
                              static_cast<quint16>(static_cast<quint8>(buffer[1]) << 8);
                rawData.push_back(val);
            }
        }
        file.close();
        // 获取cv的mat包括灰度图与GRBG图
        getCVMatGRBG();

        // 获取qImg
        getQImage();

    } else{
        qImg = QImage(filePath);
        width = qImg.width();
        height = qImg.height();
        // 设置界面显示信息
        ui->lineEdit_width->setText(QString::number(width));
        ui->lineEdit_height->setText(QString::number(height));
        // TODO: 转为cvMat执行cv操作
        img_mat_root = qImageToCVMat(qImg);
    }
    // 显示
    ui->label->setPixmap(QPixmap::fromImage(qImg));
    // 高斯模糊
    getCVMatGaussian();
    circles.clear();

    // 启用按钮
    ui->pushButton_opencv->setDisabled(false);
    ui->pushButton_opencv_gray->setDisabled(false);
    ui->pushButton_circle->setDisabled(false);
    ui->pushButton_canny->setDisabled(false);
    ui->pushButton_gray->setDisabled(false);
    ui->pushButton_save->setDisabled(false);
    ui->pushButton_threshold->setDisabled(false);
    ui->pushButton_labelImg_gray8->setDisabled(false);

    ui->spinBox_brightness->setDisabled(false);
    ui->doubleSpinBox_contrast->setDisabled(false);
    ui->horizontalSlider_brightness->setDisabled(false);
    ui->horizontalSlider_contrast->setDisabled(false);
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
    img_mat_root = display_image;

}

void Widget::getCVMatGaussian()
{
    if("raw" == imgType){
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
    }else{
        if(img_mat_root.type() == CV_8UC1)
            img_mat_gray = img_mat_root.clone();

        else
            cv::cvtColor(img_mat_root, img_mat_gray, cv::COLOR_BGRA2GRAY);
    }
    // 降噪（高斯模糊）
    cv::GaussianBlur(img_mat_root, img_mat_gaussian, cv::Size(5, 5), 2, 2);
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

void Widget::on_pushButton_opencv_clicked()
{
    // 显示图片
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_root)));
    // 显示图像
    //cv::namedWindow("RAW Image", cv::WINDOW_NORMAL);
    //cv::imshow("RAW Image", img_mat_grbg);
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

    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_gaussian)));
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

cv::Mat Widget::qImageToCVMat(const QImage &qImage)
{
    qDebug() << "format : " << qImage.format();
    QImage image = qImage;

    // 根据QImage格式进行转换
    switch (image.format()) {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        // 处理32位格式（包含Alpha通道）
        image = image.convertToFormat(QImage::Format_ARGB32);
        break;
    case QImage::Format_RGB888:
        // 直接处理24位RGB格式
        break;
    case QImage::Format_Indexed8:
    case QImage::Format_Grayscale8:
        // 处理8位灰度图
        image = image.convertToFormat(QImage::Format_Grayscale8);
        break;
    default:
        // 其他格式转换为32位ARGB
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    // 创建对应类型的cv::Mat
    cv::Mat mat;
    const int channels = image.isGrayscale() ? 1 : (image.hasAlphaChannel() ? 4 : 3);

    if (image.format() == QImage::Format_RGB888) {
        // 处理24位RGB888格式
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR); // 转换RGB到BGR
    } else if (channels == 4) {
        // 处理32位带Alpha的图像
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR); // 转换BGRA到BGR并去除Alpha
    } else if (channels == 1) {
        // 处理8位灰度图
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
    }

    // 确保返回的Mat数据连续且独立
    return mat.clone();
}

void Widget::on_pushButton_circle_clicked()
{

    // 只需要执行一次
    if(circles.empty()){
        // 绘制圆形的mat
        img_mat_circle = img_mat_root.clone();
        // 高斯处理灰度图
        cv::Mat gaussion_gray;
        cv::GaussianBlur(img_mat_gray, gaussion_gray, cv::Size(5, 5), 2, 2);

        // 霍夫圆检测
        cv::HoughCircles(
            gaussion_gray,           // 输入灰度图像
            circles,        // 输出结果（x, y, radius）
            cv::HOUGH_GRADIENT, // 检测方法（目前仅支持梯度法）
            1,              // 累加器分辨率（与图像尺寸的倒数，通常为1）
            gaussion_gray.rows/64,    // 圆之间的最小距离（避免重复检测）
            200,            // Canny边缘检测的高阈值
            100,            // 累加器阈值（越小检测越多假圆）
            0,              // 最小圆半径（0表示不限制）
            0);              // 最大圆半径（0表示不限制）

        if(circles.empty()){
            QMessageBox::warning(nullptr, "不存在圆点", "未找到圆点，请检查图片");
            img_mat_circle = cv::Mat();
            return;
        }
        // 更新圆点顺序(逆时针排序)
        sortCirclePoint();
        // 设置圆点信息
        setCircleInfo();
        std::vector<cv::Point> centers;
        // 绘制检测结果
        for (size_t i = 0; i < circles.size(); i++) {
            qDebug() << circles[i][0] << "," << circles[i][1] << "半径：" << circles[i][2];
            cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            centers.push_back(center);
            int radius = cvRound(circles[i][2]);
            // 绘制实心圆（红色填充）
            cv::circle(
                img_mat_circle,                    // 目标图像
                center,                 // 圆心坐标
                radius,                 // 半径
                cv::Scalar(0, 0, 255),  // 颜色（BGR格式，红色）
                -1                      // 线宽：-1 表示填充
                );
            // 绘制圆心
            cv::circle(img_mat_circle, center, 3, cv::Scalar(0, 255, 0), -1);
            cv::Point text_pos(center.x + 60, center.y);
            // 绘制圆序号
            cv::putText(
                img_mat_circle,
                QString::number(i + 1).toStdString(),
                text_pos,
                cv::FONT_HERSHEY_SIMPLEX,
                1,
                cv::Scalar(0, 0, 255), // 红色
                5
                );
        }
        double dis13;
        double dis24;
        cvLine(centers[0], centers[2], dis13);
        cvLine(centers[1], centers[3], dis24);
        ui->lineEdit_circle13_dis->setText(QString::number(dis13));
        ui->lineEdit_circle24_dis->setText(QString::number(dis24));
    }
    // 显示绘制图
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_circle)));
}

void Widget::sortCirclePoint(){
    // 计算四边形的中心点
    double c_x = 0, c_y = 0;
    for (const auto& p : circles) {
        c_x += p[0];
        c_y += p[1];
    }
    c_x /= 4;
    c_y /= 4;
    // 按逆时针方向排序（使用叉积避免浮点运算）
    sort(circles.begin(), circles.end(), [&c_x, &c_y](const cv::Vec3f& a, const cv::Vec3f& b) {
        // 计算相对于中心点的向量
        int ax = a[0] - c_x;
        int ay = a[1] - c_y;
        int bx = b[0] - c_x;
        int by = b[1] - c_y;

        // 通过叉积判断方向
        int cross = ax * by - ay * bx;
        if (cross != 0) return cross > 0; // 逆时针排序
        // 共线时按距离排序（确保相邻点顺序正确）
        return (ax * ax + ay * ay) < (bx * bx + by * by);
    });
}

void Widget::init()
{
    savePath = "";
    filePath = "";
    width = 0;
    height = 0;
    img_mat_root = cv::Mat();
    img_mat_gray = cv::Mat();
    img_mat_gaussian = cv::Mat();
    img_mat_circle = cv::Mat();
    img_mat_canny = cv::Mat();
    qImg = QImage();

    rawData.clear();
    circles.clear();
    imgType = "";

    ui->label->clear();
}

void Widget::setCircleInfo()
{
    ui->lineEdit_circle1_x->setText(QString::number(circles[0][0]));
    ui->lineEdit_circle1_y->setText(QString::number(circles[0][1]));
    ui->lineEdit_circle1_r->setText(QString::number(circles[0][2]));

    ui->lineEdit_circle2_x->setText(QString::number(circles[1][0]));
    ui->lineEdit_circle2_y->setText(QString::number(circles[1][1]));
    ui->lineEdit_circle2_r->setText(QString::number(circles[1][2]));

    ui->lineEdit_circle3_x->setText(QString::number(circles[2][0]));
    ui->lineEdit_circle3_y->setText(QString::number(circles[2][1]));
    ui->lineEdit_circle3_r->setText(QString::number(circles[2][2]));

    ui->lineEdit_circle4_x->setText(QString::number(circles[3][0]));
    ui->lineEdit_circle4_y->setText(QString::number(circles[3][1]));
    ui->lineEdit_circle4_r->setText(QString::number(circles[3][2]));
}

void Widget::cvLine(cv::Point pt1, cv::Point pt2, double &distance){
    // 绘制直线（红色，线宽2像素）
    cv::line(
        img_mat_circle,         // 目标图像
        pt1,                    // 起点坐标
        pt2,                    // 终点坐标
        cv::Scalar(0, 0, 255),  // 颜色（BGR格式，此处为红色）
        5                       // 线宽（像素）
        );
    // 计算两点距离
    distance = cv::norm(pt1 - pt2); // 使用OpenCV的范数计算

    // 格式化距离文本（保留两位小数）
    std::stringstream ss;
    ss << "Distance: " << std::fixed << std::setprecision(2) << distance << " px";
    std::string text = ss.str();
    // 计算文本位置（直线中点）
    cv::Point text_pos(
        (pt2.x - pt1.x) / 3 + pt1.x, // 三等分点
        (pt2.y - pt1.y) / 3 + pt1.y  //
        );

    //绘制距离文本
    cv::putText(
        img_mat_circle,
        text,
        text_pos,
        cv::FONT_HERSHEY_SIMPLEX,
        1,
        cv::Scalar(255, 0, 0), // 蓝色
        4
        );
}

void Widget::on_pushButton_reset_clicked()
{
    ui->label->resetPos(QPoint(0, 0));
}


void Widget::on_pushButton_canny_clicked()
{

    cv::Canny(img_mat_gaussian, img_mat_canny, 100, 200); // 使用相同阈值
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_canny)));
}


void Widget::on_pushButton_gray_clicked()
{
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(img_mat_gray)));
}


void Widget::on_pushButton_save_clicked()
{
    savePath = "";
    savePath = QFileDialog::getSaveFileName(this,
                                              "请选择保存位置",
                                              "undifined.png",
                                              "Image Files (*.jpg;*.png);");
    if(savePath.isEmpty()) return;
    ui->label->pixmap()->save(savePath);
}


void Widget::on_pushButton_threshold_clicked()
{
    threshType = (threshType + 1) % 2;
    cv::Mat thresh_Mat;
    int t_type = 0;
    if(threshType){
        // 二进制二值化
        t_type = cv::THRESH_BINARY;
        ui->pushButton_threshold->setText("反二值化");
    }
    else{
        // 二进制反二值化
        t_type = cv::THRESH_BINARY_INV;
        ui->pushButton_threshold->setText("二值化");
    }
    // cv::threshold(输入图像, 输出图像, 阈值, 最大值, 阈值类型)
    // 输入图像必须单通道，最大值通常设置为255
    cv::threshold(img_mat_gray, thresh_Mat, 100, 255, t_type);
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(thresh_Mat)));
}
/* 亮度对比度调节 */
void Widget::changeGain(double contrast, int brightness)
{
    cv::Mat changedGainMat;
    // 计算公式是 对比度 * 像素值 + 亮度
    img_mat_root.convertTo(changedGainMat, -1, contrast, brightness);
    ui->label->setPixmap(QPixmap::fromImage(cvMatToQImage(changedGainMat)));
}

void Widget::on_spinBox_brightness_valueChanged(int bri)
{
    ui->horizontalSlider_brightness->setSliderPosition(bri);
    double c = ui->doubleSpinBox_contrast->value();
    changeGain(c, bri);
}


void Widget::on_horizontalSlider_brightness_valueChanged(int value)
{
    ui->spinBox_brightness->setValue(value);
    double c = ui->doubleSpinBox_contrast->value();
    changeGain(c, value);
}


void Widget::on_doubleSpinBox_contrast_valueChanged(double con)
{
    ui->horizontalSlider_contrast->setSliderPosition(con * 10);
    int b = ui->spinBox_brightness->value();
    changeGain(con, b);
}


void Widget::on_horizontalSlider_contrast_valueChanged(int value)
{
    double con = static_cast<double>(value) / 10;
    ui->doubleSpinBox_contrast->setValue(con);
    int b = ui->spinBox_brightness->value();
    changeGain(con, b);
}

