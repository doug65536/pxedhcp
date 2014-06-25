CONFIG += qt
CONFIG += console
CONFIG += warn_on
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11
SOURCES = main.cpp mainwindow.cpp pxeresponder.cpp pxeservice.cpp tftpserver.cpp tftptransfer.cpp
HEADERS = mainwindow.h pxeresponder.h pxeservice.h tftpserver.h tftptransfer.h
FORMS = mainwindow.ui
QT += network

QT += core
QT += gui
QT += widgets

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

OTHER_FILES += \
    android/AndroidManifest.xml
