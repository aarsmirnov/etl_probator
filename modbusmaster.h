#pragma once

#include <QEventLoop>
#include <QBitArray>
#include <QModbusRtuSerialMaster>
#include <QPair>
#include <QSerialPort>
#include <QVariant>

class ModbusMaster : public QModbusRtuSerialMaster
{
    Q_OBJECT

public:
    enum ModbusError {
        NO_ERROR = 0,
        COM_CONNECTION_ERROR,
        NO_RESPONSE_FROM_MODBUS_SLAVE,
        INTERNAL_INVALID_MODBUS_INPUT_DATA,
        INTERNAL_MODBUS_MODULE_ERROR,
        UNKNOWN_MODBUS_ERROR
    };
    Q_ENUM(ModbusError)

    static QMap<ModbusError, QString> ModbusErrorText;

    enum Command {
        c_NO_COMMAND,
        c_AUTO,
        c_MANUAL,
        c_SAVE,
        c_RESET,
        c_ERROR = 99
    };

    enum Status {
        s_AUTO,
        s_MANUAL,
        s_DOWNTIME,
        s_STOP,
        s_ERROR = 99
    };

    QMap<QString, QSerialPort::Parity> qstr_to_parity_map = {
        { "No parity", QSerialPort::NoParity },
        { "Even parity", QSerialPort::EvenParity },
        { "Odd parity", QSerialPort::OddParity },
        { "Space parity", QSerialPort::SpaceParity },
        { "Mark parity", QSerialPort::MarkParity }
    };

    QMap<ModbusMaster::ModbusError, QString> err_to_qstr = {
        { NO_ERROR, "NO_ERROR" },
        { COM_CONNECTION_ERROR, "COM_CONNECTION_ERROR" },
        { NO_RESPONSE_FROM_MODBUS_SLAVE, "NO_RESPONSE_FROM_MODBUS_SLAVE" },
        { INTERNAL_INVALID_MODBUS_INPUT_DATA, "INTERNAL_INVALID_MODBUS_INPUT_DATA" },
        { INTERNAL_MODBUS_MODULE_ERROR, "INTERNAL_MODBUS_MODULE_ERROR" },
        { UNKNOWN_MODBUS_ERROR, "UNKNOWN_MODBUS_ERROR" }
    };

    ModbusMaster(QObject* parent = nullptr);

    QBitArray decToQBitArray(const quint16& cur_val); //не вызовет ли передача по адресу проблемы ?
    quint16 QBitArrayToDec(const QBitArray& cur_array);

    ModbusError connectModbus();
    void waitForModbusReply(const QModbusReply* reply);

    QPair<int, ModbusMaster::ModbusError> readBit(int cur_bit_address);
    QPair<QBitArray, ModbusMaster::ModbusError> readRegister(int cur_register_address);
    ModbusMaster::ModbusError writeBit(int cur_bit_address, bool cur_bit_state);
    ModbusMaster::ModbusError writeRegister(int cur_register_address, QBitArray cur_bit_array_val);
    QPair<QPair<QString, QString>, ModbusError> readID();
    ModbusMaster::ModbusError changeAddress(quint8 new_lab_address);
    QPair<ModbusMaster::Status, ModbusMaster::ModbusError> readStatus();
    ModbusMaster::ModbusError writeCommand(ModbusMaster::Command cur_command);


    void setModbusLabParameters(int lab_port,
                                int lab_bauds,
                                int lab_stop_bits,
                                int lab_data_bits,
                                QString lab_parity,
                                int lab_response_time,
                                int lab_modbus_net_addr);

private:
    const int UNDEFINED = 99; //используется для неопределённых значений
    const quint16 ONE_BIT_OR_REGISTER = 1; //используется для конфигурации считывания одного бита или байта

    const int MODBUS_TRY_SEND_COUNT = 2; //число попыток при отправке данных на slave Modbus

    const int RECONNECT_COM_COUNT = 3; //число попыток при переподключении к COM

    const int STATUS_ADDRES = 5; //регистр со значнием статуса
    const int COMMAND_ADDRES = 2; //регистр со значнием команды

    //параметры modbus по умолчанию
    QVariant LAB_PORT = "COM1";
    QVariant LAB_BAUDS = 19200;
    QVariant LAB_STOP_BITS = 1;
    QVariant LAB_DATA_BITS = 8;
    QVariant LAB_PARITY = QSerialPort::NoParity;

    int LAB_RESPONSE_TIME = 500; //время таймаута ожидания одного ответа от slave modbus (min 10)
    int LAB_MODBUS_NET_ADDRESS = 1; //адрес устройства в modbus сети
};
