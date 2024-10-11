#include "v100_modbusmaster.h"
//#include "logfile.h"
//#include <QStringConverter>
#include <QByteArray>


//QMap<ModbusMaster::ModbusError, QString> ModbusMaster::ModbusErrorText = {
//    {ModbusMaster::ModbusError::NO_ERROR, QObject::tr("Корректная работа ModBus модуля программы.")},
//    {ModbusMaster::ModbusError::COM_CONNECTION_ERROR, QObject::tr("Ошибка подключения к COM порту.")},
//    {ModbusMaster::ModbusError::NO_RESPONSE_FROM_MODBUS_SLAVE, QObject::tr("Нет ответа от устройства по текущему адресу.")},
//    {ModbusMaster::ModbusError::INTERNAL_INVALID_MODBUS_INPUT_DATA, QObject::tr("Некорректная передача параметров в ModBus.")},
//    {ModbusMaster::ModbusError::INTERNAL_MODBUS_MODULE_ERROR, QObject::tr("Некорректная работа ModBus модуля программы.")},
//    {ModbusMaster::ModbusError::UNKNOWN_MODBUS_ERROR, QObject::tr("Неизвестная ошибка ModBus.")},
//};

QMap<V100ModbusMaster::ModbusError, QString> V100ModbusMaster::ModbusErrorText = {
    {V100ModbusMaster::ModbusError::NO_ERROR, QObject::tr("ModBus module works OK.")},
    {V100ModbusMaster::ModbusError::COM_CONNECTION_ERROR, QObject::tr("Error connecting to COM port.")},
    {V100ModbusMaster::ModbusError::NO_RESPONSE_FROM_MODBUS_SLAVE, QObject::tr("No response on the current slave address.")},
    {V100ModbusMaster::ModbusError::INTERNAL_INVALID_MODBUS_INPUT_DATA, QObject::tr("Error in ModBus input data.")},
    {V100ModbusMaster::ModbusError::INTERNAL_MODBUS_MODULE_ERROR, QObject::tr("ModBus module error.")},
    {V100ModbusMaster::ModbusError::UNKNOWN_MODBUS_ERROR, QObject::tr("Unknown ModBus error.")},
};

V100ModbusMaster::V100ModbusMaster(QObject* parent)
    : QModbusRtuSerialMaster(parent)
{
//    QModbusResponse::registerDataSizeCalculator(
//        QModbusPdu::FunctionCode(0x34),
//        [](const QModbusResponse&) -> int {
//            return 5;
//        });
//    QModbusResponse::registerDataSizeCalculator(
//        QModbusPdu::FunctionCode(0x35),
//        [](const QModbusResponse&) -> int {
//            return 4;
//        });
}

void V100ModbusMaster::setModbusLabParameters(int lab_port,
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

void V100ModbusMaster::waitForModbusReply(const QModbusReply *reply)
{
//    Logger.Log("Waiting for Modbus reply");
    QEventLoop waitingLoop;
    QTimer waitingtimer;
    auto wait = [&]()
    {
        waitingLoop.quit();
    };

    connect(&waitingtimer, &QTimer::timeout, this, wait);

    connect(reply, &QModbusReply::finished, &waitingLoop, &QEventLoop::quit);
    connect(reply, &QModbusReply::errorOccurred, &waitingLoop, &QEventLoop::quit);

    waitingtimer.start(LAB_RESPONSE_TIME/2);
    waitingLoop.exec();
    reply->disconnect();
//    Logger.Log("Modbus reply waiting finished");
}

V100ModbusMaster::ModbusError V100ModbusMaster::connectModbus()
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
//              Logger.Log(QString("COM connect attempt %1").arg(counter));
            counter++;
        } else {
            return COM_CONNECTION_ERROR;
        }
    }
//    Logger.Log("Modbus connected");
    return NO_ERROR;
}

QPair<int, V100ModbusMaster::ModbusError> V100ModbusMaster::readBit(int cur_bit_address)
{
 //   Logger.Log(QString("ModbusMaster::readBit(%1)").arg(cur_bit_address));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(UNDEFINED, COM_CONNECTION_ERROR);
        }
    }
    auto bit_data_unit = QModbusDataUnit(QModbusDataUnit::Coils, cur_bit_address, V100ModbusMaster::ONE_BIT_OR_REGISTER);
    auto* cur_reply = QModbusClient::sendReadRequest(bit_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
    cur_reply = QModbusClient::sendReadRequest(bit_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
            return qMakePair(V100ModbusMaster::UNDEFINED, NO_RESPONSE_FROM_MODBUS_SLAVE);
        }
        //причина ошибки не выявлена
        return qMakePair(V100ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(V100ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
}

QPair<QBitArray, V100ModbusMaster::ModbusError> V100ModbusMaster::readRegister(int cur_register_address)
{
//    Logger.Log(QString("ModbusMaster::readRegister(%1)").arg(cur_register_address));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(QBitArray(), COM_CONNECTION_ERROR);
        }
    }
    auto register_data_unit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, cur_register_address, V100ModbusMaster::ONE_BIT_OR_REGISTER);
    auto* cur_reply = QModbusClient::sendReadRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
    cur_reply = QModbusClient::sendReadRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
       // return qMakePair(ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
        return qMakePair(QBitArray(), UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(QBitArray(), UNKNOWN_MODBUS_ERROR);
}

V100ModbusMaster::ModbusError V100ModbusMaster::writeBit(int cur_bit_address, bool cur_bit_state)
{
//    Logger.Log(QString("ModbusMaster::writeBit(%1, %2)").arg(cur_bit_address).arg(cur_bit_state));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return COM_CONNECTION_ERROR;
        }
    }
    const QVector<quint16> cur_data_to_write(1, (quint16)cur_bit_state);
    auto bit_data_unit = QModbusDataUnit(QModbusDataUnit::Coils, cur_bit_address, cur_data_to_write);
    auto* cur_reply = QModbusClient::sendWriteRequest(bit_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
    cur_reply = QModbusClient::sendWriteRequest(bit_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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

V100ModbusMaster::ModbusError V100ModbusMaster::writeRegister(int cur_register_address, QBitArray cur_bit_array_val)
{
//    Logger.Log(QString("ModbusMaster::writeRegister(%1, %2)").arg(cur_register_address).arg(BitsToString(cur_bit_array_val)));
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
    auto* cur_reply = QModbusClient::sendWriteRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
    cur_reply = QModbusClient::sendWriteRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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

QPair<V100ModbusMaster::Status, V100ModbusMaster::ModbusError> V100ModbusMaster::readStatus()
{
//    Logger.Log(QString("ModbusMaster::readStatus"));
    QPair<QBitArray, V100ModbusMaster::ModbusError> read_register_result = readRegister(STATUS_ADDRES);
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
    V100ModbusMaster::Status status_enum = s_ERROR;
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

V100ModbusMaster::ModbusError V100ModbusMaster::writeCommand(V100ModbusMaster::Command cur_command)
{
    QBitArray command_val = decToQBitArray(cur_command);
    return writeRegister(COMMAND_ADDRES, command_val);
}


// Чтение номера модели и серийного номера установки
QPair<QString, V100ModbusMaster::ModbusError> V100ModbusMaster::readSerial()
 {
    QString result;
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(result, COM_CONNECTION_ERROR);
        }
    }
    QModbusRequest cur_request_unit(QModbusPdu::ReportServerId); // код функции 0х11 параметров нет
    auto* cur_reply = sendRawRequest(cur_request_unit, LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        const QModbusResponse cur_modbus_response = cur_reply->rawResult(); // Тут глюит с ответом - надо разобраться почему так!
        //if (cur_reply->error() == QModbusDevice::NoError
            //&& cur_modbus_response.isValid()
            //&&
        //    cur_modbus_response.dataSize() == 29 // Приняли ожидаемое число байт+1, потому что размер+размер данных
            //&& cur_modbus_response.functionCode() == 0x11 // проверить
        //   ) { // Условия проверки пакета закончились
            //ответ был получен, ошибок нет, формат ответа верный
        QByteArray msg(cur_modbus_response.data().remove(0,1).toStdString().c_str());
         //Вот эта хрень не работает как надо. строка там в 1251
//        auto toUtf16 = QStringDecoder(QStringDecoder::System);
//        result = toUtf16(msg);
               return qMakePair(msg, NO_ERROR);
        //}
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return qMakePair(result, COM_CONNECTION_ERROR);
    }
    cur_reply = sendRawRequest(cur_request_unit, LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        const QModbusResponse cur_modbus_response = cur_reply->rawResult();
        //if (cur_reply->error() == QModbusDevice::NoError
        //        && cur_modbus_response.isValid()
        //        && cur_modbus_response.dataSize() == 29 // Приняли ожидаемое число байт+1, потому что 10+размер данных
        //        && cur_modbus_response.functionCode() == 0x11 // проверить
        //        ) { // Условия проверки пакета закончились
            //ответ был получен, ошибок нет, формат ответа верный
            //result.append(cur_modbus_response.data().remove(0,1));
        //QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
        //result = codec->toUnicode(cur_modbus_response.data().remove(0,1));
            //должны добавить строку, удалив первый символ, который размер данных
            return qMakePair(cur_modbus_response.data().remove(0,1), NO_ERROR);
        //}
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return qMakePair(result, NO_RESPONSE_FROM_MODBUS_SLAVE);
        }
        //причина ошибки не выявлена
        return qMakePair(result, UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(result, UNKNOWN_MODBUS_ERROR);
}

QBitArray V100ModbusMaster::decToQBitArray(const quint16 cur_val)
{
    QBitArray cur_bit_array = QBitArray((int)16, false);
    for (quint8 iter { 0 }; iter < 16; iter++) {
        if (cur_val & (1 << iter)) {
            cur_bit_array.setBit(iter, true);
        }
    }
    return cur_bit_array;
}

quint16 V100ModbusMaster::QBitArrayToDec(const QBitArray& cur_array)
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

// Функция должна прочитать регистры с 0 по 9 адрес и срыгнуть их значения в виде массива.
QPair<QVector<quint16>, V100ModbusMaster::ModbusError> V100ModbusMaster:: dataRead()
{
    QVector<quint16> cur_modbus_data_vector;
    //    Logger.Log(QString("ModbusMaster::readRegister(%1)").arg(cur_register_address));
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return qMakePair(cur_modbus_data_vector, COM_CONNECTION_ERROR);
        }
    }
    auto register_data_unit = QModbusDataUnit(QModbusDataUnit::InputRegisters, 0, READ_AT_ONCE);
    auto* cur_reply = QModbusClient::sendReadRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == QModbusDevice::NoError && cur_reply->result().isValid()) {
            cur_modbus_data_vector = cur_reply->result().values();
            if (!cur_modbus_data_vector.isEmpty()) {
                //ответ был получен, ошибок нет, формат ответа верный
                //quint16& cur_modbus_data = cur_modbus_data_vector.first(); это не используется тк возвращаем весь массив
                return qMakePair(cur_modbus_data_vector, NO_ERROR);
            }
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return qMakePair(cur_modbus_data_vector, COM_CONNECTION_ERROR);
    }
    cur_reply = QModbusClient::sendReadRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        QVector<quint16> cur_modbus_data_vector = cur_reply->result().values();
        if (cur_reply->error() == QModbusDevice::NoError
            && cur_reply->result().isValid() && !cur_modbus_data_vector.isEmpty()) {
            //ответ был получен, ошибок нет, формат ответа верный
            //quint16& cur_modbus_data = cur_modbus_data_vector.first();
            return qMakePair(cur_modbus_data_vector, NO_ERROR);
        }
        if (cur_reply->error() == QModbusDevice::TimeoutError) {
            //master смог отправить запрос в Modbus сеть
            //но slave с указанным адресом не откликнулся
            return qMakePair(cur_modbus_data_vector, NO_RESPONSE_FROM_MODBUS_SLAVE);
        }
        //причина ошибки не выявлена
        // return qMakePair(ModbusMaster::UNDEFINED, UNKNOWN_MODBUS_ERROR);
        return qMakePair(cur_modbus_data_vector, UNKNOWN_MODBUS_ERROR);
    }
    //причина ошибки не выявлена
    return qMakePair(cur_modbus_data_vector, UNKNOWN_MODBUS_ERROR);
}

//Тут запись группы регистров для последующей записи настроек, до 30 за раз.
V100ModbusMaster::ModbusError V100ModbusMaster::writeParams(int cur_register_address, QVector<quint16> cur_Vector_val)
{
    //    Logger.Log(QString("ModbusMaster::writeRegister(%1, %2)").arg(cur_register_address).arg(BitsToString(cur_bit_array_val)));

   // if (cur_bit_array_val.isNull())
    if (cur_Vector_val.size() == 0 || cur_Vector_val.size() > 30)
    {
        //нет ни одного элемента в массиве или их больше 30
        return INTERNAL_INVALID_MODBUS_INPUT_DATA;
    }
 //   if (cur_register_address < 0x000B) {
 //       //Попытались записать в регистры только для чтения или регистр команды
 //       return INTERNAL_INVALID_MODBUS_INPUT_DATA;
 //   }
    if (QModbusDevice::state() != ConnectedState) {
        if (connectModbus() != NO_ERROR) {
            //COM порт при вызове метода не был подключен
            //переподключиться к COM порту не получилось
            return COM_CONNECTION_ERROR;
        }
    }
    //WriteMultipleRegisters
    auto register_data_unit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, cur_register_address, cur_Vector_val); // Запись нескольких регистров параметров функция 0х10, начальный адрес и параметры.
    auto cur_reply = QModbusClient::sendWriteRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    // auto* cur_reply = QModbusClient::sendRawRequest(request,ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    if (cur_reply) {
        waitForModbusReply(cur_reply);
        if (cur_reply->error() == NoError && cur_reply->result().isValid()) {
            //ответ был получен, ошибок нет, формат ответа верный
            return NO_ERROR;
        }
        //
        int er = cur_reply->error();
        if (er == 6) {
            //ответ был получен, ошибок нет, формат ответа верный
            return INVALID_DEVICE_DATA;
        }
    }
    if (connectModbus() != NO_ERROR) {
        //скорее всего, произошло "горячее отключение" COM
        //переподключение не помогло
        return COM_CONNECTION_ERROR;
    }
    cur_reply = QModbusClient::sendWriteRequest(register_data_unit, V100ModbusMaster::LAB_MODBUS_NET_ADDRESS);
    //cur_reply = QModbusClient::sendRawRequest(request,ModbusMaster::LAB_MODBUS_NET_ADDRESS);
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
        int er = cur_reply->error();
        if (er == 6) {
            //ответ был получен, ошибок нет, формат ответа верный
            return INVALID_DEVICE_DATA;
        }
        //причина ошибки не выявлена
        return UNKNOWN_MODBUS_ERROR;
    }
    //причина ошибки не выявлена
    return UNKNOWN_MODBUS_ERROR;
}

void V100ModbusMaster::disConnectModbus(void)
{
    QModbusDevice::disconnectDevice();
}
