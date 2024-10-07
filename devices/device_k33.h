#ifndef DEVICE_K33_H
#define DEVICE_K33_H

#include <QWidget>
#include <QStandardItemModel>

#include "device.h"

namespace Ui {
class Device_K33;
}

class Device_K33 : public Device
{
    Q_OBJECT

public:
    explicit Device_K33(const QString &title, const QPixmap &schema, Device *parent = nullptr);
    ~Device_K33();

    QString name() const override { return QStringLiteral("Коэффициент 3.3"); }
    QVector<QStringList> protocol() override;

private:
    void configUi();
    void setParamMode(bool enable);

private:
    Ui::Device_K33 *ui;

    QStandardItemModel  *m_terminalModel;
    QStandardItemModel  *m_ktProtocolModel;
};

#endif // DEVICE_K33_H
