#-------------------------------------------------
#
# Project created by QtCreator 2013-06-23T19:16:29
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tile_editor
TEMPLATE = app


SOURCES += main.cpp\
    tiledownloader.cpp \
    maputils.cpp \
    filedownloader.cpp

HEADERS  += \
    tiledownloader.h \
    maputils.h \
    filedownloader.h

FORMS    += \
    tiledownloader.ui

RESOURCES += \
    resources.qrc

windows: {
    message(Windows desktop build)
}
