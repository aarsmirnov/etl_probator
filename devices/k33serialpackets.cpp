#include "k33serialpackets.h"

#include <QMap>

QByteArray K33Packets::makeDeviceInfoPacket()
{
    QByteArray result;
    result.append(GetDeviceInfo);
    result.append(static_cast<char>(0));
    result.append(static_cast<char>(0));

    return result;
}

QByteArray K33Packets::makeSetMeasureParamsPacket(const MeasureParams &params)
{
    QByteArray result;
    result.append(SetMeasureParams);
    result.append(static_cast<char>(0));
    result.append(static_cast<char>(0));

    result.append(params.mode_code);
    result.append(params.voltage_code);

    result.append(params.u_a);
    result.append(params.u_b);
    result.append(params.u_c);
    result.append(params.i_a);
    result.append(params.i_b);
    result.append(params.i_c);

    result.append(static_cast<char>(0));
    result.append(static_cast<char>(1));

    result[2] = result.size() + 1;

    return result;
}

QByteArray K33Packets::makeStartMeasurePacket()
{
    QByteArray result;
    result.append(StartMeasure);
    result.append(static_cast<char>(0));
    result.append(static_cast<char>(0));

    return result;
}


QMap<QString, QString> K33Packets::parseMeasureResultPacket(const QByteArray &data, uint8_t mode)
{
    const QByteArray payload = data.mid(4);
    const auto payloadParts = payload.split('\0');

    switch (mode) {
    case 0x00:
        return parseMeasureKtResultPacket(payloadParts);
    case 0x01:
        return parseMeasureHhResultPacket(payloadParts);
    case 0x02:
        return parseMeasureKzResultPacket(payloadParts);
    default:
        break;
    }

    return QMap<QString, QString>();
}

QMap<QString, QString> K33Packets::parseMeasureKtResultPacket(const QList<QByteArray> &data)
{
    int index = 18;
    QMap<QString, QString> result;
    result["fba"]  = QString::fromUtf8(data[index++]);
    result["fbb"]  = QString::fromUtf8(data[index++]);
    result["fbc"]  = QString::fromUtf8(data[index++]);
    result["fna"]  = QString::fromUtf8(data[index++]);
    result["fnb"]  = QString::fromUtf8(data[index++]);
    result["fnc"]  = QString::fromUtf8(data[index++]);
    result["ubab"] = QString::fromUtf8(data[index++]);
    result["ubbc"] = QString::fromUtf8(data[index++]);
    result["ubca"] = QString::fromUtf8(data[index++]);
    result["unab"] = QString::fromUtf8(data[index++]);
    result["unbc"] = QString::fromUtf8(data[index++]);
    result["unca"] = QString::fromUtf8(data[index++]);
    result["kkt"]  = QString::fromUtf8(data[index++]);
    result["ktab"] = QString::fromUtf8(data[index++]);
    result["ktbc"] = QString::fromUtf8(data[index++]);
    result["ktca"] = QString::fromUtf8(data[index++]);
    return result;
}

QMap<QString, QString> K33Packets::parseMeasureHhResultPacket(const QList<QByteArray> &data)
{
    int index = 18;
    QMap<QString, QString> result;
    result["fua"]  = QString::fromUtf8(data[index++]);
    result["fub"]  = QString::fromUtf8(data[index++]);
    result["fuc"]  = QString::fromUtf8(data[index++]);
    result["fia"]  = QString::fromUtf8(data[index++]);
    result["fib"]  = QString::fromUtf8(data[index++]);
    result["fic"]  = QString::fromUtf8(data[index++]);
    result["uab"]  = QString::fromUtf8(data[index++]);
    result["ubc"]  = QString::fromUtf8(data[index++]);
    result["uca"]  = QString::fromUtf8(data[index++]);
    result["ia"]   = QString::fromUtf8(data[index++]);
    result["ib"]   = QString::fromUtf8(data[index++]);
    result["ic"]   = QString::fromUtf8(data[index++]);
    result["pa"]   = QString::fromUtf8(data[index++]);
    result["pb"]   = QString::fromUtf8(data[index++]);
    result["pc"]   = QString::fromUtf8(data[index++]);
    result["xxa"]  = QString::fromUtf8(data[index++]);
    result["xxb"]  = QString::fromUtf8(data[index++]);
    result["xxc"]  = QString::fromUtf8(data[index++]);
    result["xa"]   = QString::fromUtf8(data[index++]);
    result["xb"]   = QString::fromUtf8(data[index++]);
    result["xc"]   = QString::fromUtf8(data[index++]);
    return result;
}

QMap<QString, QString> K33Packets::parseMeasureKzResultPacket(const QList<QByteArray> &data)
{
    int index = 18;
    QMap<QString, QString> result;
    result["fua"]  = QString::fromUtf8(data[index++]);
    result["fub"]  = QString::fromUtf8(data[index++]);
    result["fuc"]  = QString::fromUtf8(data[index++]);
    result["fia"]  = QString::fromUtf8(data[index++]);
    result["fib"]  = QString::fromUtf8(data[index++]);
    result["fic"]  = QString::fromUtf8(data[index++]);
    result["uab"]  = QString::fromUtf8(data[index++]);
    result["ubc"]  = QString::fromUtf8(data[index++]);
    result["uca"]  = QString::fromUtf8(data[index++]);
    result["ia"]   = QString::fromUtf8(data[index++]);
    result["ib"]   = QString::fromUtf8(data[index++]);
    result["ic"]   = QString::fromUtf8(data[index++]);
    result["zab"]  = QString::fromUtf8(data[index++]);
    result["zbc"]  = QString::fromUtf8(data[index++]);
    result["zca"]  = QString::fromUtf8(data[index++]);
    return result;
}
