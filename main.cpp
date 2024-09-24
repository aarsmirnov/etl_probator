#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QMessageBox>
#include <QScreen>
#include <QThread>
#include <QDebug>

#include "mainwidget.h"
#include "core.h"
#include "logfile.h"

extern "C"{
bool T2000init(uint8_t port);
bool T2000status(int *val);
bool T2000statusd();
uint8_t T2000key(uint8_t val);
uint8_t T2000GenVD();
uint8_t T2000mode();
char *T2000line1();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QObject::tr("ЭТЛ SCAN-2"));
    QApplication::setAttribute(Qt::AA_Use96Dpi);

//    auto res = T2000init(3);
//    int value = 0;
//    uint8_t keyRes = T2000key('\0');
//    keyRes = T2000key('\0');

////    res = T2000status(&value);
////    uint8_t res1 = T2000mode();
////    char *text = nullptr;
////    text = T2000line1();

////    QThread::sleep(2);

////    res = T2000statusd();
////    char *text = nullptr;
////    text = T2000line1();

//    while (1) {
//        int value = 0;
//        QThread::sleep(1);
//        res = T2000status(&value);

//        if(value >= 0) {
//            qDebug() << value;
//        }
//    }



    QSharedPointer<Core> programm_core;
    try {
        programm_core = QSharedPointer<Core>::create();
    } catch (const std::exception &ex) {
        Logger.Log(ex.what());
        QMessageBox::critical(nullptr, QObject::tr("Error"), ex.what());
        return EXIT_FAILURE;
    }

    MainWidget mainWidget(programm_core);
    QObject::connect(&mainWidget, &MainWidget::exitTriggered, &app, &QApplication::quit);

    mainWidget.setWindowFlags(Qt::FramelessWindowHint);
    //mainWidget.setWindowState(mainWidget.windowState() | Qt::WindowFullScreen);
    mainWidget.show();

    return app.exec();
}
