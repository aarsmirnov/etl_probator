#ifndef BLUETOOTHIKSDEVICE_H
#define BLUETOOTHIKSDEVICE_H

#include <QObject>
#include <qbluetoothserviceinfo.h>
#include <QLowEnergyController>

class BluetoothIKSDevice : public QObject
{
    Q_OBJECT

public:
    enum Command {
        Test,
        ActiveResistance,
        InductiveResistance
    };

    explicit BluetoothIKSDevice(const QBluetoothDeviceInfo &info, QObject *parent = nullptr);
    ~BluetoothIKSDevice();

    bool connect();
    void disconnect();

    void sendRequest(Command cmd, const QVariant param = QVariant());

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void responseReceived(const QVariant &response);

private slots:
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onServiceCharacteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value);
    void onServiceError(QLowEnergyService::ServiceError error);

private:
    QLowEnergyController        *m_controller { nullptr };
    QLowEnergyService           *m_service { nullptr };
    QLowEnergyCharacteristic    m_rxCharacteristic;
};

#endif // BLUETOOTHIKSDEVICE_H
