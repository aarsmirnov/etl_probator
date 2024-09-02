#ifndef DEVICE_IKS30A_H
#define DEVICE_IKS30A_H

#include <QWidget>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothuuid.h>

#include <QtSvg/QSvgWidget>

#include "device.h"
#include "bluetoothiksdevice.h"

namespace Ui {
class Device_IKS30A;
}

class Device_IKS30A : public Device
{
    Q_OBJECT

public:
    explicit Device_IKS30A(const QString &title, Device *parent = nullptr);
    ~Device_IKS30A();

    QString name() const override { return QStringLiteral("ИКС30А"); }

private:
    void configUi();

private:
    Ui::Device_IKS30A *ui;

    QSvgWidget                          *m_spinner { nullptr };
    QSvgWidget                          *m_spinner2 { nullptr };
    QBluetoothDeviceDiscoveryAgent      *m_discoveryAgent { nullptr };
    BluetoothIKSDevice                  *m_controller { nullptr };
    QMap<QString, QBluetoothDeviceInfo> m_devices;
};

#endif // DEVICE_IKS30A_H
