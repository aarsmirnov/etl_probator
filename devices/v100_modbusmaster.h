#pragma once

#include <QEventLoop>
#include <QBitArray>
#include <QModbusRtuSerialMaster>
#include <QModbusDataUnit>
#include <QPair>
#include <QSerialPort>
#include <QVariant>
#include <QTimer>
#include <QSettings>

class V100ModbusMaster : public  QModbusRtuSerialMaster
{
    Q_OBJECT

public:
    enum ModbusError {
        NO_ERROR = 0,
        COM_CONNECTION_ERROR,
        NO_RESPONSE_FROM_MODBUS_SLAVE,
        INTERNAL_INVALID_MODBUS_INPUT_DATA,
        INTERNAL_MODBUS_MODULE_ERROR,
        UNKNOWN_MODBUS_ERROR,
        INVALID_DEVICE_DATA
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

    QMap<V100ModbusMaster::ModbusError, QString> err_to_qstr = {
        { NO_ERROR, "NO_ERROR" },
        { COM_CONNECTION_ERROR, "COM_CONNECTION_ERROR" },
        { NO_RESPONSE_FROM_MODBUS_SLAVE, "NO_RESPONSE_FROM_MODBUS_SLAVE" },
        { INTERNAL_INVALID_MODBUS_INPUT_DATA, "INTERNAL_INVALID_MODBUS_INPUT_DATA" },
        { INTERNAL_MODBUS_MODULE_ERROR, "INTERNAL_MODBUS_MODULE_ERROR" },
        { UNKNOWN_MODBUS_ERROR, "UNKNOWN_MODBUS_ERROR" },
        { INVALID_DEVICE_DATA,  "INVALID DEVICE DATA" }
    };
 //  typedef struct
 //   {
//        QString Model;
//        QString Version;
//        QString Serial;
//    } MSN;

    V100ModbusMaster(QObject* parent = nullptr);

    QBitArray decToQBitArray(const quint16 cur_val); //не вызовет ли передача по адресу проблемы ? Вызвала!
    quint16 QBitArrayToDec(const QBitArray& cur_array);

    ModbusError connectModbus(); // подключение к изделию
    void waitForModbusReply(const QModbusReply* reply);
    void disConnectModbus(void); // отключение от изделия

    QPair<int, V100ModbusMaster::ModbusError> readBit(int cur_bit_address);
    QPair<QBitArray, V100ModbusMaster::ModbusError> readRegister(int cur_register_address);
    V100ModbusMaster::ModbusError writeBit(int cur_bit_address, bool cur_bit_state);
    V100ModbusMaster::ModbusError writeRegister(int cur_register_address, QBitArray cur_bit_array_val);
    QPair<QString, V100ModbusMaster::ModbusError> readSerial(); // Чтение номера модели и серийного номера установки
    QPair<V100ModbusMaster::Status, V100ModbusMaster::ModbusError> readStatus(); // Чтение только одного регистра - статуса нахрен загадка...
    V100ModbusMaster::ModbusError writeCommand(V100ModbusMaster::Command cur_command);
    QPair<QVector<quint16>, V100ModbusMaster::ModbusError> dataRead(); // Функция должна прочитать регистры с 0 по 9 адрес и срыгнуть их значения в виде массива.
    V100ModbusMaster::ModbusError writeParams(int cur_register_address, QVector<quint16> cur_Vector_val); //Тут запись группы регистров для последующей записи настроек, до 20 за раз.

    void setModbusLabParameters(int lab_port,
                                int lab_bauds,
                                int lab_stop_bits,
                                int lab_data_bits,
                                QString lab_parity,
                                int lab_response_time,
                                int lab_modbus_net_addr);

private:
    QTimer*   waitingtimer;

    const int UNDEFINED = 99; //используется для неопределённых значений
    const quint16 ONE_BIT_OR_REGISTER = 1; //используется для конфигурации считывания одного бита или байта

    const int MODBUS_TRY_SEND_COUNT = 2; //число попыток при отправке данных на slave Modbus

    const int RECONNECT_COM_COUNT = 3; //число попыток при переподключении к COM

    const int STATUS_ADDRES = 0x0000; //регистр со значнием статуса
    const int COMMAND_ADDRES = 0x000A; //регистр со значнием команды

    //параметры modbus по умолчанию
    QVariant LAB_PORT = "COM1";
    QVariant LAB_BAUDS = 19200;
    QVariant LAB_STOP_BITS = 2;
    QVariant LAB_DATA_BITS = 8;
    QVariant LAB_PARITY = QSerialPort::NoParity;

#define READ_AT_ONCE 30 // сколько регистров читаем за раз для получения оператичных данных о состоянии установки

    int LAB_RESPONSE_TIME = 250; //время таймаута ожидания одного ответа от slave modbus (min 10)
    int LAB_MODBUS_NET_ADDRESS = 3; //адрес устройства в modbus сети
};
