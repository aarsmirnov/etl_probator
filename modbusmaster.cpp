#include "modbusmaster.h"
#include "logfile.h"

//QMap<ModbusMaster::ModbusError, QString> ModbusMaster::ModbusErrorText = {
//    {ModbusMaster::ModbusError::NO_ERROR, QObject::tr("Корректная работа ModBus модуля программы.")},
//    {ModbusMaster::ModbusError::COM_CONNECTION_ERROR, QObject::tr("Ошибка подключения к COM порту.")},
//    {ModbusMaster::ModbusError::NO_RESPONSE_FROM_MODBUS_SLAVE, QObject::tr("Нет ответа от устройства по текущему адресу.")},
//    {ModbusMaster::ModbusError::INTERNAL_INVALID_MODBUS_INPUT_DATA, QObject::tr("Некорректная передача параметров в ModBus.")},
//    {ModbusMaster::ModbusError::INTERNAL_MODBUS_MODULE_ERROR, QObject::tr("Некорректная работа ModBus модуля программы.")},
//    {ModbusMaster::ModbusError::UNKNOWN_MODBUS_ERROR, QObject::tr("Неизвестная ошибка ModBus.")},
//};

QMap<ModbusMaster::ModbusError, QString> ModbusMaster::ModbusErrorText = {
    {ModbusMaster::ModbusError::NO_ERROR, QObject::tr("ModBus module works OK.")},
    {ModbusMaster::ModbusError::COM_CONNECTION_ERROR, QObject::tr("Error connecting to COM port.")},
    {ModbusMaster::ModbusError::NO_RESPONSE_FROM_MODBUS_SLAVE, QObject::tr("No response on the current slave address.")},
    {ModbusMaster::ModbusError::INTERNAL_INVALID_MODBUS_INPUT_DATA, QObject::tr("Error in ModBus input data.")},
    {ModbusMaster::ModbusError::INTERNAL_MODBUS_MODULE_ERROR, QObject::tr("ModBus module error.")},
    {ModbusMaster::ModbusError::UNKNOWN_MODBUS_ERROR, QObject::tr("Unknown ModBus error.")},
};

ModbusMaster::ModbusMaster(QObject* parent)
    : QModbusRtuSerialMaster(parent)
{
    QModbusResponse::registerDataSizeCalculator(
        QModbusPdu::FunctionCode(0x34),
        [](const QModbusResponse&) -> int {
            return 5;
        });
    QModbusResponse::registerDataSizeCalculator(
        QModbusPdu::FunctionCode(0x35),
        [](const QModbusResponse&) -> int {
            return 4;
        });
}

void ModbusMaster::setModbusLabParameters(int lab_port,
    int lab_bauds,
    int lab_stop_bits,
    int lab_data_bits,
    QString lab_parity,
    int lab_response_time,
    int lab_modbus_net_addr)
{
    LAB_PORT = QString("COM%1").arg(lab_port);
    LAB_BAUDS = lab_bauds;
    LAB_STOP_BITS = lab_stop_bits;
    LAB_DATA_BITS = lab_data_bits;
    LAB_PARITY = qstr_to_parity_map.value(lab_parity, QSerialPort::NoParity);
    LAB_RESPONSE_TIME = lab_response_time;
    LAB_MODBUS_NET_ADDRESS = lab_modbus_net_addr;
}

void ModbusMaster::waitForModbusReply(const QModbusReply *reply)
{
    Logger.Log("Waiting for Modbus reply");
    QEventLoop waitingLoop;
    connect(reply, &QModbusReply::finished, &waitingLoop, &QEventLoop::quit);
    waitingLoop.exec();
    reply->disconnect();
    Logger.Log("Modbus reply waiting finished");
}

ModbusMaster::ModbusError ModbusMaster::connectModbus()
{
    int counter { 0 };
    if (QModbusDevice::state() == QModbusDevice::ConnectedState) {
        QModbusDevice::disconnectDevice();
    }
    QModbusDevice::setConnectionParameter(
        QModbusDevice::SerialPortNameParameter, LAB_PORT);
    setConnectionParameter(SerialParityParameter, LAB_PARITY);
    setConnectionParameter(SerialBaudRateParameter, LAB_BAUDS);
    setConnectionParameter(SerialDataBitsParameter, LAB_DATA_BITS);
    setConnectionParameter(SerialStopBitsParameter, LAB_STOP_BITS);
    QModbusClient::setTimeout(LAB_RESPONSE_TIME);
    QModbusClient::setNumberOfRetries(MODBUS_TRY_SEND_COUNT - 1); //(отсчёт идёт с 0, поэтому -1)
    while (!QModbusDevice::connectDevice()) {
        if (counter < RECONNECT_COM_COUNT - 1) {
              Logger.Log(QString("COM connect attempt %1").arg(counter));
            counter++;
        } else {
            return COM_CONNECTION_ERROR;
        }
    }
    Logger.Log("Modbus connected");
    return NO_ERROR;
}

QPair<int, ModbusMaster::ModbusError> ModbusMaster::readBit(int cur_bit_address)
{
    Logger.Log(QString("ModbusMaster::readBit(%1)").arg(cur_bit_address));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(UNDEFINED, COM_CONNECTION_ERROR);
        }
    }
    auto bit_data_unit = QModbusDataUnit(QModbusDataUnit::Coils, cur_bit_address, ModbusMaster::ONE_BIT_OR_REGISTER);
    auto* cur_reply = QModbusClient::sendReadRequest(bit_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return qMakePair(cur_reply->result().value(0), NO_ERROR);
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return qMakePair(UNDEFINED, COM_CONNECTION_ERROR);
    }
    //повторная отправка запроса
    cur_reply = QModbusClient::sendReadRequest(bit_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError
                && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return qMakePair(cur_reply->result().value(0), NO_ERROR);
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return qMakePair(ModbusMaster::UNDEFINED, NO_RESPONSE_FROM_MODBUS_SLAVE);
        }
        //причина ошибки не выявлена
        return qMakePair(ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
}

QPair<QBitArray, ModbusMaster::ModbusError> ModbusMaster::readRegister(int cur_register_address)
{
    Logger.Log(QString("ModbusMaster::readRegister(%1)").arg(cur_register_address));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(QBitArray(), COM_CONNECTION_ERROR);
        }
    }
    auto register_data_unit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, cur_register_address, ModbusMaster::ONE_BIT_OR_REGISTER);
    auto* cur_reply = QModbusClient::sendReadRequest(register_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError && cur_reply->result().isValid()) {
            QVector<quint16> cur_modbus_data_vector = cur_reply->result().values();
            if (!cur_modbus_data_vector.isEmpty()) {
                //ответ был получен, ошибок нет, формат ответа верный
                quint16& cur_modbus_data = cur_modbus_data_vector.first();
                return qMakePair(decToQBitArray(cur_modbus_data), NO_ERROR);
            }
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return qMakePair(QBitArray(), COM_CONNECTION_ERROR);
    }
    cur_reply = QModbusClient::sendReadRequest(register_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        QVector<quint16> cur_modbus_data_vector = cur_reply->result().values();
        if (cur_reply->error() == QModbusDevice::NoError
                && cur_reply->result().isValid() && !cur_modbus_data_vector.isEmpty()) {
            //ответ был получен, ошибок нет, формат ответа верный
            quint16& cur_modbus_data = cur_modbus_data_vector.first();
            return qMakePair(decToQBitArray(cur_modbus_data), NO_ERROR);
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return qMakePair(QBitArray(), NO_RESPONSE_FROM_MODBUS_SLAVE);
        }
        //причина ошибки не выявлена
        return qMakePair(ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(QBitArray(), UNKNOWN_MODBUS_ERROR);
}

ModbusMaster::ModbusError ModbusMaster::writeBit(int cur_bit_address, bool cur_bit_state)
{
    Logger.Log(QString("ModbusMaster::writeBit(%1, %2)").arg(cur_bit_address).arg(cur_bit_state));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return COM_CONNECTION_ERROR;
        }
    }
    const QVector<quint16> cur_data_to_write(1, (quint16)cur_bit_state);
    auto bit_data_unit = QModbusDataUnit(QModbusDataUnit::Coils, cur_bit_address, cur_data_to_write);
    auto* cur_reply = QModbusClient::sendWriteRequest(bit_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return NO_ERROR;
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return COM_CONNECTION_ERROR;
    }
    cur_reply = QModbusClient::sendWriteRequest(bit_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return NO_ERROR;
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return NO_RESPONSE_FROM_MODBUS_SLAVE;
        }
        //причина ошибки не выявлена
        return UNKNOWN_MODBUS_ERROR;

    }
    //причина ошибки не выявлена
    return UNKNOWN_MODBUS_ERROR;
}

ModbusMaster::ModbusError ModbusMaster::writeRegister(int cur_register_address, QBitArray cur_bit_array_val)
{
    Logger.Log(QString("ModbusMaster::writeRegister(%1, %2)").arg(cur_register_address).arg(BitsToString(cur_bit_array_val)));
    if (cur_bit_array_val.isNull()) {
        //нет ни одного элемента в массиве
        return INTERNAL_INVALID_MODBUS_INPUT_DATA;
    }
    if (cur_bit_array_val.size() != 16) {
        //регистр имеет постоянный размер в 16 бит
        return INTERNAL_INVALID_MODBUS_INPUT_DATA;
    }
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return COM_CONNECTION_ERROR;
        }
    }
    const QVector<quint16> cur_data_to_write(1, QBitArrayToDec(cur_bit_array_val));
    auto register_data_unit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, cur_register_address, cur_data_to_write);
    auto* cur_reply = QModbusClient::sendWriteRequest(register_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == NoError && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return NO_ERROR;
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return COM_CONNECTION_ERROR;
    }
    cur_reply = QModbusClient::sendWriteRequest(register_data_unit, ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError
                && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return NO_ERROR;
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return NO_RESPONSE_FROM_MODBUS_SLAVE;
        }
        //причина ошибки не выявлена
        return UNKNOWN_MODBUS_ERROR;
    }
    //причина ошибки не выявлена
    return UNKNOWN_MODBUS_ERROR;
}

QPair<ModbusMaster::Status, ModbusMaster::ModbusError> ModbusMaster::readStatus()
{
    Logger.Log(QString("ModbusMaster::readStatus"));
    QPair<QBitArray, ModbusMaster::ModbusError> read_register_result = readRegister(STATUS_ADDRES);
    if (read_register_result.second != NO_ERROR) {
        //при использовании метода считывания регистра была ошибка
        return qMakePair(s_ERROR, read_register_result.second);
    }
    if (read_register_result.first.isNull()) {
        //пришёл пустой массив, внутреняя ошибка
        return qMakePair(s_ERROR, INTERNAL_MODBUS_MODULE_ERROR);
    }
    if (read_register_result.first.size() != 16) {
        //пришёл массив неверного размера, внутреняя ошибка
        return qMakePair(s_ERROR, INTERNAL_MODBUS_MODULE_ERROR);
    }
    quint16 status_result = QBitArrayToDec(read_register_result.first);
    ModbusMaster::Status status_enum = s_ERROR;
    switch (status_result) {
    case 0:
        status_enum = s_AUTO;
        break;
    case 1:
        status_enum = s_MANUAL;
        break;
    case 2:
        status_enum = s_DOWNTIME;
        break;
    case 3:
        status_enum = s_STOP;
        break;
    }
    return qMakePair(status_enum, read_register_result.second);
}

ModbusMaster::ModbusError ModbusMaster::writeCommand(ModbusMaster::Command cur_command)
{
    QBitArray command_val = decToQBitArray(cur_command);
    return writeRegister(COMMAND_ADDRES, command_val);
}

QPair<QPair<QString, QString>, ModbusMaster::ModbusError> ModbusMaster::readID()
{
    QPair<QString, QString> result_pair;
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(result_pair, COM_CONNECTION_ERROR);
        }
    }
    QModbusRequest cur_request_unit(QModbusPdu::FunctionCode(0x34), (quint16)0, (quint16)0);
    auto* cur_reply = sendRawRequest(cur_request_unit, LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        const QModbusResponse cur_modbus_response = cur_reply->rawResult();
        if (cur_reply->error() == QModbusDevice::NoError
            && cur_modbus_response.isValid()
            && cur_modbus_response.dataSize() == 5
            && cur_modbus_response.data().toHex().size() == 10
            && cur_modbus_response.data().toHex().at(1) == *"4") {
            //ответ был получен, ошибок нет, формат ответа верный
            result_pair.first.setNum(
                cur_modbus_response.data().toHex().remove(0, 2).remove(4, 4).toInt(nullptr, 16), 10);
            result_pair.second.setNum(
                cur_modbus_response.data().toHex().remove(0, 2).remove(0, 4).toInt(nullptr, 16), 10);
            return qMakePair(result_pair, NO_ERROR);
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return qMakePair(result_pair, COM_CONNECTION_ERROR);
    }
    cur_reply = sendRawRequest(cur_request_unit, LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        const QModbusResponse cur_modbus_response = cur_reply->rawResult();
        if (cur_reply->error() == QModbusDevice::NoError
                && cur_modbus_response.isValid()
                && cur_modbus_response.dataSize() == 5
                && cur_modbus_response.data().toHex().size() == 10
                && cur_modbus_response.data().toHex().at(1) == *"4") {
            //ответ был получен, ошибок нет, формат ответа верный
            result_pair.first.setNum(
                        cur_modbus_response.data().toHex().remove(0, 2).remove(4, 4).toInt(nullptr, 16), 10);
            result_pair.second.setNum(
                        cur_modbus_response.data().toHex().remove(0, 2).remove(0, 4).toInt(nullptr, 16), 10);
            return qMakePair(result_pair, NO_ERROR);
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return qMakePair(result_pair, NO_RESPONSE_FROM_MODBUS_SLAVE);
        }
        //причина ошибки не выявлена
        return qMakePair(result_pair, UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(result_pair, UNKNOWN_MODBUS_ERROR);
}

//небезопастный метод, так как при появлении ошибок, адрес устройства считается прежним,
//что может не соотвествовать действительности, нужны доп. проверки, использовать осторожно
ModbusMaster::ModbusError ModbusMaster::changeAddress(quint8 new_lab_address)
{
    if (new_lab_address >= 247 || new_lab_address < 1) {
        return INTERNAL_INVALID_MODBUS_INPUT_DATA;
    }
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return COM_CONNECTION_ERROR;
        }
    }
    QModbusRequest cur_request_unit(QModbusPdu::FunctionCode(0x35), (quint8)0, (quint8)0, (quint8)0, new_lab_address);
    auto* cur_reply = sendRawRequest(cur_request_unit, LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        const QModbusResponse cur_modbus_response = cur_reply->rawResult();
        if (cur_reply->error() == QModbusDevice::NoError
            && cur_modbus_response.dataSize() == 4
            && cur_modbus_response.data().toHex().toLong(nullptr, 16) == new_lab_address) {
            //ответ был получен, ошибок нет, формат ответа верный
            LAB_MODBUS_NET_ADDRESS = (int)new_lab_address;
            return NO_ERROR;
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return COM_CONNECTION_ERROR;
    }
    cur_reply = sendRawRequest(cur_request_unit, LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        const QModbusResponse cur_modbus_response = cur_reply->rawResult();
        if (cur_reply->error() == QModbusDevice::NoError
                && cur_modbus_response.dataSize() == 4
                && cur_modbus_response.data().toHex().toLong(nullptr, 16) == new_lab_address) {
            //ответ был получен, ошибок нет, формат ответа верный
            LAB_MODBUS_NET_ADDRESS = (int)new_lab_address;
            return NO_ERROR;
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return NO_RESPONSE_FROM_MODBUS_SLAVE;
        }
        //причина ошибки не выявлена
        return UNKNOWN_MODBUS_ERROR;
    }
    //причина ошибки не выявлена
    return UNKNOWN_MODBUS_ERROR;
}

QBitArray ModbusMaster::decToQBitArray(const quint16& cur_val)
{
    QBitArray cur_bit_array = QBitArray((int)16, false);
    for (quint8 iter { 0 }; iter < 16; iter++) {
        if (cur_val & (1 << iter)) {
            cur_bit_array.setBit(iter, true);
        }
    }
    return cur_bit_array;
}

quint16 ModbusMaster::QBitArrayToDec(const QBitArray& cur_array)
{
    if (cur_array.size() != 16) {
        return (quint16)0;
    }
    quint16 cur_val = cur_array.at(0);
    for (quint8 iter { 1 }; iter < 16; iter++) {
        cur_val |= cur_array.at(iter) << iter;
    }
    return cur_val;
}

