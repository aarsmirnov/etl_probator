#ifndef K33SERIALPACKETS_H
#define K33SERIALPACKETS_H

#include <QByteArray>
#include <QString>

namespace K33Packets {
    enum Command {
        SetLimit            = 0x01,
        GetAllLimits        = 0x02,
        SetVoltageMode      = 0x03,
        SetMeasureParams    = 0x20,
        StartMeasure        = 0x21,
        GetDeviceInfo       = 0xFF,
    };

    enum Mode {
        KT = 0x01,
        HH = 0x02,
        KZ = 0x03,
    };

    struct MeasureParams {
        uint8_t mode_code { 0x00 };
        uint8_t voltage_code { 0x00 };
        uint8_t u_a { 0x00 };
        uint8_t u_b { 0x00 };
        uint8_t u_c { 0x00 };
        uint8_t i_a { 0x00 };
        uint8_t i_b { 0x00 };
        uint8_t i_c { 0x00 };
    };

    struct MeasureResult {
        QString fba;
        QString fbb;
        QString fbc;
        QString fna;
        QString fnb;
        QString fnc;
        QString ubab;
        QString ubbc;
        QString ubca;
        QString unab;
        QString unbc;
        QString unca;
        QString kkt;
        QString ktab;
        QString ktbc;
        QString ktca;
    };

    QByteArray makeDeviceInfoPacket();
    QByteArray makeSetMeasureParamsPacket(const MeasureParams &params);

    QByteArray makeStartMeasurePacket();

    QMap<QString, QString> parseMeasureResultPacket(const QByteArray &data, uint8_t mode);

    QMap<QString, QString> parseMeasureKtResultPacket(const QList<QByteArray> &data);
    QMap<QString, QString> parseMeasureHhResultPacket(const QList<QByteArray> &data);
    QMap<QString, QString> parseMeasureKzResultPacket(const QList<QByteArray> &data);
}



#endif // K33SERIALPACKETS_H
