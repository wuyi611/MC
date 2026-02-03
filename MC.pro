QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    StreamWidget.cpp \
    capturethread.cpp \
    main.cpp \
    processthread.cpp

HEADERS += \
    StreamWidget.h \
    capturethread.h \
    framequeue.h \
    processthread.h

FORMS += \
    StreamWidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# OpenCV4.10
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/OpenCV4.10/x64/vc16/lib/ -lopencv_img_hash4100 -lopencv_world4100
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/OpenCV4.10/x64/vc16/lib/ -lopencv_img_hash4100d -lopencv_world4100d

INCLUDEPATH += $$PWD/OpenCV4.10/include
DEPENDPATH += $$PWD/OpenCV4.10/include

RESOURCES += \
    res.qrc
