TEMPLATE = app
TARGET = probator

QT       += core gui serialport serialbus bluetooth widgets

CONFIG += c++11
CONFIG -= console

VERSION = 1.0.0.0
QMAKE_TARGET_COMPANY = Group of companies Energoskan
QMAKE_TARGET_PRODUCT = On-board computer laboratory
QMAKE_TARGET_DESCRIPTION = Laboratory functional management software
QMAKE_TARGET_COPYRIGHT = Group of companies Energoskan
RC_ICONS = "icon.ico"

SOURCES += \
    core.cpp \
    logfile.cpp \
    main.cpp \
    mainwidget.cpp \
    modbusmaster.cpp \
    widgets/custombutton.cpp

HEADERS += \
    core.h \
    logfile.h \
    mainwidget.h \
    modbusmaster.h \
    widgets/custombutton.h

FORMS += \
    mainwidget.ui

DEFINES += DEV_MODE # for GUI development

RESOURCES += \
    resources.qrc

CONFIG(release, debug|release) {
    DESTDIR = $$PWD/../bin
} else {
    DESTDIR = $$PWD/../bin_debug
}

win32 {
    DEFINES += BUILDDATE=\\\"$$system(date /t)\\\"
    LIBS += -luser32 -lBthprops
} else {
    DEFINES += BUILDDATE=\\\"$$system(date +%Y)\\\"
}

LIBS += -luser32 -lBthprops
