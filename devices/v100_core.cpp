#include "v100_core.h"
#include "v100_modbusmaster.h"


#include <QSettings>
#include <QMetaEnum>
#include <QPair>
#include <QUuid>
#include <QTimer>

//#include "logfile.h"

QMap<V100Core::TCoreModes, QString> V100Core::IndicationModes ={
    {V100Core::TCoreModes::m_OFF,QObject::tr("Отключено.")},
    {V100Core::TCoreModes::m_MANUAL_ON,QObject::tr("Отключено.")},
    {V100Core::TCoreModes::m_AUTO_ON,QObject::tr("Отключено.")},
    {V100Core::TCoreModes::m_DISCHARGE,QObject::tr("Отключено.")},
    {V100Core::TCoreModes::m_NOTCONNECTED,QObject::tr("Отключено.")},
    {V100Core::TCoreModes::m_ERROR,QObject::tr("Отключено.")}
};

QMap<V100Core::ERRORS, QString> V100Core::IndicationErrors = {
    {V100Core::ERRORS::er_DOOR_ERROR,QObject::tr("Внешняя блокировка разомкнута.")},
    {V100Core::ERRORS::er_UART_ERROR,QObject::tr("Ошибка связи с платой ВВ блока.")},
    {V100Core::ERRORS::er_LATR_ERROR,QObject::tr("ЛАТР не становится в ноль.")},
    {V100Core::ERRORS::er_LATR_OFF_ERROR,QObject::tr("Нет напряжения с ЛАТРа.")},
    {V100Core::ERRORS::er_PROBOY_ERROR,QObject::tr("Пробой в объекте испытания.")},
    {V100Core::ERRORS::er_DEAD_ERROR,QObject::tr("Отказ трансформатора.")},
    {V100Core::ERRORS::er_UART_ERROR_CONST,QObject::tr("Не читаются калибровки измерителя")},
    {V100Core::ERRORS::er_RESTART_ERROR,QObject::tr("Измерительный блок перезапускается.")},
    {V100Core::ERRORS::er_DIODE_ERROR,QObject::tr("Пробит диод или не работает коммутатор (большой переменный ток в режиме постоянного напряжения.")},
    {V100Core::ERRORS::er_OVERHEAT_ERROR,QObject::tr("Перегрев высоковольтного трансформатора.")},
    {V100Core::ERRORS::er_LOW_TEMP_ERROR,QObject::tr("Высоковольтный трансформатор слишком холодный.")},
    {V100Core::ERRORS::er_LOW_HV_ERROR,QObject::tr("Нет высокого напряжения.")},
    {V100Core::ERRORS::er_OVERVOLTAGE_ERROR,QObject::tr("Высокое напряжение больше ожидаемого.")},
    {V100Core::ERRORS::er_DC_ON_HV_ERROR,QObject::tr("При испытании AC на выходе диод. Ошибка коммутатора")},
    {V100Core::ERRORS::er_LATR_WRONG_DIR_ERROR,QObject::tr("Неправильное направление привода ЛАТРА.")}
};

V100Core::V100Core(QObject *parent): QObject(parent), m_modbus_computer(new V100ModbusMaster(this))
{
    CoreModes = V100Core::m_NOTCONNECTED;
    m_polling_timer = new QTimer(this);
//Тут не корректно и не происходит подключения!
    QObject::connect(m_polling_timer, &QTimer::timeout, this, &V100Core::poller);
    //connect(m_polling_timer, &QTimer::timeout, this, &Core::poller);

    // тут надо было задать конфигурацию порта из настроек, но сейчас задаём её из инперфейса пользователя



    //Тут идут проверки подключения программы к реальной установке
    // но мы подключаемся из другой функции.



}

V100Core::~V100Core()
{
    m_polling_timer->stop();
}

// начало соединения
void V100Core::m_connect(int port, int bauds, int stop_bits, int data_bits, QString parity, int response_time, int modbus_net_addr)
{

m_modbus_computer->setModbusLabParameters(port, bauds, stop_bits, data_bits, parity, response_time, modbus_net_addr);

#ifdef DEV_MODE
emit V100Core::UpdateDeviceInfo("ВИУ-100      ver._1.15_ 23010"); // отправляем данные в форму

//return;
#endif

#ifndef DEV_MODE
    //Подключение к порту
    ModbusMaster::ModbusError connection_answer = m_modbus_computer->connectModbus();
    if(connection_answer != ModbusMaster::ModbusError::NO_ERROR)
        throw std::runtime_error(ModbusMaster::ModbusErrorText[connection_answer].toUtf8().data());

#ifdef NO_SERIAL
emit Core::UpdateDeviceInfo("Virtual"); // отправляем данные в форму
#else
    // Получаем модель, статус и серийный номер
    QPair<QString, ModbusMaster::ModbusError> DeviceInfo = m_modbus_computer->readSerial();
    if(DeviceInfo.second != ModbusMaster::ModbusError::NO_ERROR)
        throw std::runtime_error(QObject::tr("Get serial error!.").toUtf8().data());
            //std::runtime_error(ModbusMaster::ModbusErrorText[DeviceInfo.second].toUtf8().data());

/*
    if(DeviceInfo.first != DEVMODELNAME)
        throw std::runtime_error(QObject::tr("Устройство по указанному адресу не совместимо с программой управления.").toUtf8().data());
*/
    emit Core::UpdateDeviceInfo(DeviceInfo.first); // отправляем данные в форму
#endif
#endif

     m_polling_timer->setInterval(POLLING_INTERVAL_MS);
    m_polling_timer->start();
    CoreModes = V100Core::m_OFF;

//    emit UpdateStatus(IndicationModes[m_OFF], 1);
}

void V100Core::poller(void) // обновление данных по таймеру из установки
{
#ifndef DEV_MODE
    QPair<QVector<quint16>, ModbusMaster::ModbusError>modbus_answer = m_modbus_computer->dataRead();
    if (modbus_answer.second == ModbusMaster::ModbusError::NO_ERROR)
    {

#endif

#ifdef DEV_MODE
        // Это отладочный вариант с генерацией фейковых данных без реальной связи с установкой
        QPair<QVector<quint16>, V100ModbusMaster::ModbusError>modbus_answer;
        modbus_answer.second = V100ModbusMaster::ModbusError::NO_ERROR;
        modbus_answer.first.fill(0,28);
        modbus_answer.first[0] = AUTO_MODE;
        modbus_answer.first[1] = 0;
        modbus_answer.first[4] = 7000;
        modbus_answer.first[5] = 20000;
        modbus_answer.first[7] = 250;
        modbus_answer.first[8] = 0;
        modbus_answer.first[9] = 50;
        modbus_answer.first[13]++;


#endif
        if ((modbus_answer.first.isEmpty())||(modbus_answer.first.size()!= READ_AT_ONCE))
        { // Проверка того что хоть по размеру то, что надо
            return;
        }
        //Logger.Log(QString("Modbus answer[starting]: %1").arg(QString::number(modbus_answer2.first)));
        // Получили какой-то ответ
        // Надо его разобрать из массива значений в массив вариантов чтобы отдать сигналом ссылку на него
        InputData.mode = modbus_answer.first[0];
        InputData.status = modbus_answer.first[1];
        InputData.error_stat = modbus_answer.first[2];
        InputData.timer_mode = modbus_answer.first[3];
        InputData.U_stab = modbus_answer.first[4];
        InputData.I_limit = modbus_answer.first[5];
        InputData.U_speed_set = modbus_answer.first[6];
        InputData.temperature_HV = modbus_answer.first[7];
        InputData.U_rms = modbus_answer.first[8];
        InputData.U_max = modbus_answer.first[9];
        InputData.I_out = modbus_answer.first[10];
        InputData.P_out = modbus_answer.first[11];

        InputData.time_in_seconds = (uint32_t)(modbus_answer.first[13]<<16)+(modbus_answer.first[12]); // собираем из 2х регистров
        InputData.I_leakage = (uint32_t)(modbus_answer.first[15]<<16)+(modbus_answer.first[14]); // собираем из 2х регистров
        InputData.LATR_U = modbus_answer.first[16];
        InputData.LATR_I = modbus_answer.first[17];
        InputData.LATR_P_act = modbus_answer.first[18];
        InputData.U_proboy = modbus_answer.first[19];
        InputData.I_proboy = modbus_answer.first[20];

        InputData.padding = modbus_answer.first[21];

        InputData.U_rms_100ms.hi = modbus_answer.first[23];
        InputData.U_rms_100ms.lo = modbus_answer.first[22]; //

        InputData.U_max_100ms.hi = modbus_answer.first[25];
        InputData.U_max_100ms.lo = modbus_answer.first[24];

        InputData.I_out_100ms.hi = modbus_answer.first[26];
        InputData.I_out_100ms.lo = modbus_answer.first[27];

        // Преобразование типов uint_16 2 шт в float пройдёт не корректно потому что структура флоат не соответствует uint32_t
        // Тут надо думать как именно это сделать


        // сигнал готовности полученных данных
        emit UpdateData(InputData);
#ifndef DEV_MODE
    }else
    {
     //   Logger.Log(QString("Input register read error: %1").arg(ModbusMaster::ModbusErrorText[modbus_answer.second]));
        emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_answer.second]);
    }
#endif
}

void V100Core::start(    uint8_t Mode,             // тип напряжения 0-DC-ручное, 1-DC-авто, 2-AC-ручное, 3-АC-авто, , 4-Прожиг
                      uint16_t U,                 // напряжение испытания
                      uint16_t I,                 // Максимальный ток при котором происходит отключение
                      uint16_t U_Speed,           // Скорость поднятия напряжения
                      uint16_t timer_min,        // Время испытания минуты
                      uint8_t time_dir,          // Направление отсчета: 0 прямое,  1 - обратное
                      uint8_t off_after_timer,   // После обратного отсчета: 0 - звуковой сигнал, 1 - завершение испытания
                      uint8_t time_start        // Запуск таймера при прямом отсчете: 0 - кнопкой, 1 - авто
                      ) // Включение высокого напряжения
{
// формируем из даты пакет и отправляем в изделие
QVector<quint16> cur_val;
cur_val.clear();
uint16_t tMode = 0;

V100ModbusMaster::ModbusError modbus_error;

// 0 бит направление - 0 Вверх 1- обратный
// 1 бит авто отключение обратного отсчёта 1 -выключаем 0- звук
// 2 бит при прямом отсчёте запуск таймера 1, 0 - в ручную
if (time_dir) {tMode = (1<<0);}
if (off_after_timer) {tMode |= (1<<1);}
if (time_start) {tMode |= (1<<2);}

switch (Mode) {
    case 0: // 0-DC-ручное
    {
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeRegister(4,m_modbus_computer->decToQBitArray(4));
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
            { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
        cur_val.append(U);
        cur_val.append(I);
        cur_val.append(timer_min);
        cur_val.append(tMode);
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeParams(20, cur_val);
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
            { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
    }break;
    case 2: // 2-AC-ручное
    {
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeRegister(4,m_modbus_computer->decToQBitArray(1));
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
        cur_val.append(U);
        cur_val.append(I);
        cur_val.append(timer_min);
        cur_val.append(tMode);
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeParams(16, cur_val);
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
    }break;

    case 3: // 3-АC-авто
    {
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeRegister(4,m_modbus_computer->decToQBitArray(2));
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
        cur_val.append(U);
        cur_val.append(I);
        cur_val.append(U_Speed);
        cur_val.append(timer_min);
        cur_val.append(tMode);
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeParams(24, cur_val);
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
    }break;

    case 1: // 1-DC-авто
    {
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeRegister(4,m_modbus_computer->decToQBitArray(5));
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
        cur_val.append(U);
        cur_val.append(I);
        cur_val.append(U_Speed);
        cur_val.append(timer_min);
        cur_val.append(tMode);
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeParams(29, cur_val);
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
    }break;

    case 4: // 4-Прожиг
    {
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeRegister(4,m_modbus_computer->decToQBitArray(7));
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
        cur_val.append(U);
        cur_val.append(timer_min*60);
#ifndef DEV_MODE
        modbus_error = m_modbus_computer->writeParams(34, cur_val);
        if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
        { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
    }break;

    default:
        break;
    }
// потом даём команду запуск
#ifndef DEV_MODE
    modbus_error = m_modbus_computer->writeRegister(0,m_modbus_computer->decToQBitArray(1));
    if(modbus_error != ModbusMaster::ModbusError::NO_ERROR)
    { emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]); return;}
#endif
}

void V100Core::stop()  // Отключение высокого напряжения
{
// даём команду стоп
//    QBitArray b(16);
  //  b.setBit(0, true);
#ifndef DEV_MODE
    ModbusMaster::ModbusError modbus_error = m_modbus_computer->writeRegister(1, m_modbus_computer->decToQBitArray(1));
    if(modbus_error == ModbusMaster::ModbusError::NO_ERROR)
    {
        return; // OK
    }
   // Logger.Log(ModbusMaster::ModbusErrorText[modbus_error]);
    emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]);
#endif
}

void V100Core::SendSteps(int steps) // Отправка настроек режима испытания в установку
{
    V100ModbusMaster::ModbusError modbus_error;
#ifndef DEV_MODE
    if (steps<0)
    {
       modbus_error = m_modbus_computer->writeRegister(3, m_modbus_computer->decToQBitArray(steps*(-1)));
    }
    else
    {
       modbus_error = m_modbus_computer->writeRegister(2, m_modbus_computer->decToQBitArray(steps));
    }

    if(modbus_error == ModbusMaster::ModbusError::NO_ERROR)
    {
        return; // OK
    }
    // Logger.Log(ModbusMaster::ModbusErrorText[modbus_error]);
    emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]);
#endif
}
