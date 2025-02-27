QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    imglabel.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    imglabel.h \
    widget.h

FORMS += \
    widget.ui

INCLUDEPATH+= D:/OpenCV/opencv_MinGW4.8/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/include/
              D:/OpenCV/opencv_MinGW4.8/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/include/opencv/
              D:/OpenCV/opencv_MinGW4.8/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/include/opencv2/
LIBS+=D:/OpenCV/opencv_MinGW4.8/OpenCV-MinGW-Build-OpenCV-3.4.8-x64/x64/mingw/bin/libopencv_*.dll

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
