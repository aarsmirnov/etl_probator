#pragma once

#include <QWidget>

namespace Ui {
class MainWidget;
}

class MainWidgetPrivate;
class Core;

class MainWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(MainWidget)

public:
    explicit MainWidget(const QSharedPointer<Core> &programm_core, QWidget *parent = nullptr);
    ~MainWidget();

signals:
    void exitTriggered();

public slots:
    void setIndication(const QBitArray &bits);

protected:
    void showEvent(QShowEvent *event);

private:
    QScopedPointer<MainWidgetPrivate> d_ptr;
};
