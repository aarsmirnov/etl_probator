#include "k33serialrequester.h"

#include <QDebug>

namespace {
    ushort CRC16(uint8_t pcBlock[], int len)
    {
        ushort crc = 0x0000;
        uint8_t ci;
        for (int s = 0; s < len; s++) {
            crc ^= (ushort)(pcBlock[s] << 8);
            for (ci = 0; ci < 8; ci++) {
                if ((crc & 0x8000) == 0x8000) {
                    crc <<= 1;
                    crc ^= 0x1021;
                }
                else {
                    crc <<= 1;
                }
            }
        }
        return crc;
    }
}

K33SerialRequester::K33SerialRequester(const QString &port, QObject *parent)
    : QObject{parent}
    , m_serialPort(new QSerialPort(this))
{
    m_serialPort->setPortName(port);
    m_serialPort->setBaudRate(QSerialPort::Baud19200);
    m_serialPort->setDataBits(QSerialPort::Data8);

    QObject::connect(m_serialPort, &QSerialPort::readyRead, this, &K33SerialRequester::onNewReceivedData);
}

bool K33SerialRequester::open()
{
    return m_serialPort->open(QIODevice::ReadWrite);
}

void K33SerialRequester::close()
{
    m_serialPort->close();
}

bool K33SerialRequester::isOpen()
{
    return m_serialPort->isOpen();
}

void K33SerialRequester::writeRequest(const QByteArray &data)
{
    QByteArray result;
    result.append(0x02);
    result.append(data);

    ushort crc = CRC16(reinterpret_cast<uint8_t*>(result.data()), result.size());
    result.append((crc >> 8) & 0xFF);
    result.append(crc & 0xFF);

    m_serialPort->write(result);
}

void K33SerialRequester::onNewReceivedData()
{
    if (m_serialPort->bytesAvailable() == 0) {
        return;
    }

    QByteArray data = m_serialPort->readAll();
    const uint8_t start = data[0];
    if (!m_hasStart) {
        if (start == 0x02) {
            const ushort len = ((data[2] << 8) | data[3] & 0xFF) + 2;
            m_toReadBytes = len - data.size();
            m_response.append(data);
            m_hasStart = (m_toReadBytes > 0);
        }
    }
    else {
        m_response.append(data);
        m_toReadBytes -= data.size();
        m_hasStart = (m_toReadBytes > 0);
    }

    if (m_hasStart == false) {
        qDebug() << m_response.toHex() << m_response.size();
        const ushort responseCrc = ((m_response[m_response.size() - 2] << 8) | (m_response[m_response.size() - 1] & 0xFF));
        const ushort calcCrc = CRC16(reinterpret_cast<uint8_t*>(m_response.data()), m_response.size() - 2);
        if (responseCrc == calcCrc) {
            emit dataReceived(m_response.mid(0, m_response.size() - 2));
            qDebug() << "Valid response";
            //                const QByteArray payload = m_response.mid(4, m_response.size() - 6);
            //                const auto payloadParts = payload.split('\0');
            //                const QString deviceName = QString::fromUtf8(payloadParts.value(0));
            //                const QString serialNumber(payloadParts.value(1));
            //                const QString firmwareVersion(payloadParts.value(2));
            //                const QString buildDate(payloadParts.value(3));
            //                const QString calibrationDate(payloadParts.value(4));
            //                qDebug() << deviceName
            //                         << QString{ QString::fromStdWString(L"Зав. №: %1") }.arg(serialNumber)
            //                         << QString{ QString::fromStdWString(L"Версия ПО: %1") }.arg(firmwareVersion)
            //                         << QString{ QString::fromStdWString(L"Изготовлен: %1") }.arg(buildDate)
            //                         << QString{ QString::fromStdWString(L"Калиброван: %1") }.arg(calibrationDate);
            //            }
        }
        m_response.clear();
        m_toReadBytes = 0;
    }
}
