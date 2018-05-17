QT -= gui

QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/include

LIBS += -L$$PWD -lmjpgstreamer

SOURCES += \
        webcam_streamer_qt.cpp
