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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QObject::tr("ЭТЛ SCAN-2"));
    QApplication::setAttribute(Qt::AA_Use96Dpi);

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
