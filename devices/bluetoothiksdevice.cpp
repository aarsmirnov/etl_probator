#include "bluetoothiksdevice.h"

namespace {
    const auto serviceUuid = QBluetoothUuid(QString("{49535343-fe7d-4ae5-8fa9-9fafd205e455}"));
    const auto txUuid = QBluetoothUuid(QString("{49535343-1e4d-4bd9-ba61-23c647249616}"));
    const auto rxUuid = QBluetoothUuid(QString("{49535343-8841-43f4-a8d4-ecbe34729bb3}"));

    QByteArray buildTestMsg()
    {
        QByteArray result;
        result.append(0xAA);
        result.append(0x01);
        result.append(0x01);
        result.append(0xAA);
        result.append(0x09);
        result.append(0xFF);
        return result;
    }

    QByteArray buildMeasureMsg(bool type, float amperage)
    {
        QByteArray result;
        result.append(0xAA);
        result.append(0x04);
        result.append(0x04);
        result.append(0xAA);
        result.append(type ? 0x02 : 0x01);
        result.append(reinterpret_cast<const char*>(&amperage), sizeof(amperage));
        return result;
    }


}

BluetoothIKSDevice::BluetoothIKSDevice(const QBluetoothDeviceInfo &info, QObject *parent)
    : QObject{parent}
    , m_controller {QLowEnergyController::createCentral(info)}
{

}

BluetoothIKSDevice::~BluetoothIKSDevice()
{
    if (m_controller) {
        m_controller->disconnectFromDevice();
    }
}

bool BluetoothIKSDevice::connect()
{
    if (m_controller == nullptr || m_controller->state() != QLowEnergyController::UnconnectedState) {
        return false;
    }

    QObject::connect(m_controller, &QLowEnergyController::connected, [this] () {
        qDebug() << "BluetoothIKSDevice: controller connected";
        m_controller->discoverServices();
    });

    QObject::connect(m_controller, &QLowEnergyController::disconnected, [this] () {
        qDebug() << "BluetoothIKSDevice: controller disconnected";
        emit disconnected();
    });

    QObject::connect(m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
                     [] (QLowEnergyController::Error newError) {
        qDebug() << "BluetoothIKSDevice: controller error:" << newError;
    });

    QObject::connect(m_controller, &QLowEnergyController::serviceDiscovered, [this] (const QBluetoothUuid &newService) {
        if (newService != serviceUuid) {
            return;
        }

        qDebug() << "BluetoothIKSDevice: found service with uuid=:" << newService;

        if (m_service == nullptr) {
            m_service = m_controller->createServiceObject(newService);
            if (m_service) {
                QObject::connect(m_service,
                                 &QLowEnergyService::stateChanged,
                                 this,
                                 &BluetoothIKSDevice::onServiceStateChanged,
                                 Qt::QueuedConnection);
                QObject::connect(m_service,
                                 &QLowEnergyService::characteristicChanged,
                                 this,
                                 &BluetoothIKSDevice::onServiceCharacteristicChanged,
                                 Qt::QueuedConnection);
                QObject::connect(m_service,
                                 QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error),
                                 this,
                                 &BluetoothIKSDevice::onServiceError,
                                 Qt::QueuedConnection);
                m_service->discoverDetails();
            }
        }
    });

    QObject::connect(m_controller, &QLowEnergyController::discoveryFinished, [] () {
        qDebug() << "BluetoothIKSDevice: service discovery finished";
    });

    m_controller->connectToDevice();
    return true;
}

void BluetoothIKSDevice::disconnect()
{
    m_controller->disconnectFromDevice();
}

void BluetoothIKSDevice::onServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered) {
        const auto txCharacteristic = m_service->characteristic(txUuid);
        m_rxCharacteristic = m_service->characteristic(rxUuid);
        if (txCharacteristic.isValid() && m_rxCharacteristic.isValid()) {
            const auto notifyConfig = txCharacteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            m_service->writeDescriptor(notifyConfig, QByteArray::fromHex("0100"));
            qDebug() << "BluetoothIKSDevice: service ready";
            emit connected();
        }
    }
}

void BluetoothIKSDevice::onServiceCharacteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    Q_UNUSED(info)

    qDebug() << "BluetoothIKSDevice: service characteristic changed:" << value.toHex();
    emit responseReceived(*(reinterpret_cast<const float*>(value.mid(5).constData())));
}

void BluetoothIKSDevice::onServiceError(QLowEnergyService::ServiceError error)
{
    qDebug() << "BluetoothIKSDevice: service error:" << error;
}

void BluetoothIKSDevice::sendRequest(Command cmd, const QVariant param)
{
    QByteArray requestMsg;
    if (cmd == Test) {
        requestMsg = buildTestMsg();
    }
    else {
        requestMsg = buildMeasureMsg(cmd == InductiveResistance, param.toFloat());
    }

    if (m_rxCharacteristic.isValid()) {
        m_service->writeCharacteristic(m_rxCharacteristic, requestMsg);
    }
}

