#ifndef DEVICE_T2000_H
#define DEVICE_T2000_H

#include <QWidget>
#include <QStandardItemModel>

#include "device.h"

namespace Ui {
class Device_T2000;
}

class Device_T2000 : public Device
{
    Q_OBJECT

public:
    enum Mode {
        Direct,
        Inverse,
    };

    explicit Device_T2000(const QString &title, const QPixmap &schema, QWidget *parent = nullptr);
    ~Device_T2000();

    QString name() const override { return QStringLiteral("Тангенс-2000"); }

private:
    void configUi();
    void insertProtocolRecords();
    void removeProtocolRecords();

private:
    Ui::Device_T2000        *ui;

    QStandardItemModel      *m_protocolModel;
};

#endif // DEVICE_T2000_H
