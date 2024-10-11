TEMPLATE = app
TARGET = probator

QT       += core gui serialport serialbus widgets svg

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
    devices/bluetoothiksdevice.cpp \
    devices/device.cpp \
    devices/device_iks30a.cpp \
    devices/device_k33.cpp \
    devices/device_t2000.cpp \
    devices/device_v100.cpp \
    devices/k33serialpackets.cpp \
    devices/k33serialrequester.cpp \
    devices/v100_core.cpp \
    devices/v100_modbusmaster.cpp \
    logfile.cpp \
    main.cpp \
    mainwidget.cpp \
    modbusmaster.cpp \
    utils.cpp \
    widgets/custombutton.cpp \
    widgets/imageviewer.cpp

HEADERS += \
    core.h \
    devices/bluetoothiksdevice.h \
    devices/device.h \
    devices/device_iks30a.h \
    devices/device_k33.h \
    devices/device_t2000.h \
    devices/device_v100.h \
    devices/k33serialpackets.h \
    devices/k33serialrequester.h \
    devices/v100_core.h \
    devices/v100_modbusmaster.h \
    logfile.h \
    mainwidget.h \
    modbusmaster.h \
    utils.h \
    widgets/custombutton.h \
    widgets/imageviewer.h

FORMS += \
    devices/device_iks30a.ui \
    devices/device_k33.ui \
    devices/device_t2000.ui \
    devices/device_v100.ui \
    mainwidget.ui \
    widgets/imageviewer.ui

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
} else {
    DEFINES += BUILDDATE=\\\"$$system(date +%Y)\\\"
}
