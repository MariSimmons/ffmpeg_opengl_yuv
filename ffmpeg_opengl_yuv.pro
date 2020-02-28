QT += core gui widgets

TARGET = opengl_yuv
CONFIG += console
CONFIG -= app_bundle

CONFIG += c++11
TEMPLATE = app

LIBS += \
    -lavcodec \
    -lavutil \
    -lavdevice \
    -lavformat \
    -lswscale

SOURCES += main.cpp \
    Decoder.cpp \
    TestWidget.cpp \
    OpenGLDisplayYUV.cpp

DESTDIR = $$PWD/bin

HEADERS += \
    Decoder.h \
    TestWidget.h \
    OpenGLDisplayYUV.h

RESOURCES += \
    shaders.qrc

FORMS += \
    TestWidget.ui
