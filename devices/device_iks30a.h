#ifndef DEVICE_IKS30A_H
#define DEVICE_IKS30A_H

#include <QWidget>
#include <QTimer>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothuuid.h>

#include <QtSvg/QSvgWidget>
#include <QStandardItemModel>

#include "device.h"
#include "bluetoothiksdevice.h"

namespace Ui {
class Device_IKS30A;
}

class Device_IKS30A : public Device
{
    Q_OBJECT

public:
    explicit Device_IKS30A(const QString &title, const QPixmap &schema, Device *parent = nullptr);
    ~Device_IKS30A();

    QString name() const override { return QStringLiteral("ИКС30А"); }
    QVector<QStringList> protocol() override;

private:
    void configUi();
    void setMeasureResult(float val = -1);
    void insertProtocolRecords();
    void removeProtocolRecords();
    float getResistanceForTemp(float baseResistance);
    QStandardItemModel *getCurrentProtocolModel();

private:
    Ui::Device_IKS30A                   *ui;

    QSvgWidget                          *m_spinner { nullptr };
    QBluetoothDeviceDiscoveryAgent      *m_discoveryAgent { nullptr };
    BluetoothIKSDevice                  *m_controller { nullptr };
    QMap<QString, QBluetoothDeviceInfo> m_devices;
    QTimer                              *m_connectionTimer { nullptr };
    float                               m_lastMeasureResult { 0.0f };

    QStandardItemModel                  *m_VnProtocolModel;
    QStandardItemModel                  *m_SnProtocolModel;
    QStandardItemModel                  *m_NnProtocolModel;

    QMap<int, float>                     m_measureResult;
};

#endif // DEVICE_IKS30A_H
