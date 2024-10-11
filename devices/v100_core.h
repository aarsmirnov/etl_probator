#ifndef CORE_H
#define CORE_H

#pragma once

#include <QObject>
#include <QVariant>
#include <QBitArray>
#include <QSet>
#include <QTimer>
#include <QSettings>
#include <QMap>

//#define DEV_MODE // Нет реального подключения к плате
//#define NO_SERIAL

#define POLLING_INTERVAL_MS 100 // Это получается 4 запроса в секунду, что уже само по себе немало, можно 2

#define DEVMODELNAME "БВИ" // Имя модели к чему подключились чтобы знать что это нужная установка


QT_FORWARD_DECLARE_CLASS(QSettings);
class V100ModbusMaster;


class V100Core: public QObject
{
    Q_OBJECT

public:
    explicit V100Core(QObject *parent = nullptr);
    ~V100Core();

  enum TCoreModes
    {// режимы работы ядра программы
        m_OFF,
        m_MANUAL_ON,
        m_AUTO_ON,
        m_DISCHARGE,
        m_NOTCONNECTED,
        m_ERROR = 99
    };

    static QMap<TCoreModes, QString> IndicationModes;

// Структуры данных для работы с устновкой
typedef union{
        uint16_t hi;
        uint16_t lo;
        float  fl;
    } Tfl;


    typedef struct
    {
        uint16_t mode;              /// режим+род тока
        uint16_t status;            /// 0 - ожидание, не ноль - какая то строка в программе, типа отводится штанга (подготовка к запуску?)
            ///идет набор напряжение, стабилизация, снятие напряжение, снятие остаточного заряда и тп.
        uint16_t error_stat;        /// флаги ошибок, 0 - все ОК
        uint16_t timer_mode;        /// настройки таймера + флаг что запущен + флаг что время вышло и идет звуковой сигнал

        uint16_t U_stab;            /// *0.1 кВ, напряжение стабилизации в АВТО  /  максимальное напряжение в РУЧНОМ
        uint16_t I_limit;           /// *0,1 мА, ток отключения в АВТО, ограничения в РУЧНОМ
        uint16_t U_speed_set;       /// *0.1 кВ/с  максимальная скорость набора напряжения в АВТО

        uint16_t temperature_HV;    /// *0.1 С , температура трансформатора в высоковольтном блоке

        uint16_t U_rms;             /// действующее напряжение
        uint16_t U_max;             /// амплитудное напряжение
        uint16_t I_out;             /// выходной ток (rms в режиме AC, avg в режиме DC)
        uint16_t P_out;             /// В*А в АС, Вт в Dc , выходная мощность

        uint32_t time_in_seconds;   /// время в таймере в секундах
        uint32_t I_leakage;         /// * 1 мкА, ток утечки/ток за 10с

        uint16_t LATR_U;            /// * 0.1В
        uint16_t LATR_I;            /// * 1мА
        uint16_t LATR_P_act;        /// * 1Вт

        uint16_t U_proboy;          /// при пробое/перегрузке тут значения
        uint16_t I_proboy;

        uint16_t padding;           /// выравнивание массива
        /// для графиков, обновляются раз в 0,1сек

        Tfl U_rms_100ms;          /// кВ
        Tfl U_max_100ms;          /// кВ
        Tfl I_out_100ms;          /// мА
        /// для графиков

    } InputDataTypeDef; // Данные получаемые из установки чтением по таймеру

    InputDataTypeDef InputData;

    typedef struct // Структура настроек, которые отправляются в установку перед испытанием
    {
        uint8_t AC_DC;             // тип напряжения 1-AC, 0-DC, 2-Прожиг
        uint16_t U;                 // напряжение испытания
        uint16_t I;                 // Максимальный ток при котором происходит отключение
        uint16_t U_Speed;           // Скорость поднятия напряжения
        uint16_t timer_min;        // Время испытания минуты
        uint8_t time_dir;          // Направление отсчета: 0 прямое,  1 - обратное
        uint8_t off_after_timer;   // После обратного отсчета: 0 - звуковой сигнал, 1 - завершение испытания
        uint8_t time_start;        // Запуск таймера при прямом отсчете: 0 - кнопкой, 1 - авто
    } SetTypeDef;

    enum ERRORS
    {
        er_DOOR_ERROR = 0,
        er_UART_ERROR,
        er_LATR_ERROR,
        er_LATR_OFF_ERROR,
        er_PROBOY_ERROR,
        er_DEAD_ERROR,
        er_UART_ERROR_CONST,
        er_RESTART_ERROR,
        er_DIODE_ERROR,
        er_OVERHEAT_ERROR,
        er_LOW_TEMP_ERROR,
        er_LOW_HV_ERROR,
        er_OVERVOLTAGE_ERROR,
        er_DC_ON_HV_ERROR,
        er_LATR_WRONG_DIR_ERROR
    };
    static QMap<ERRORS, QString> IndicationErrors;

    V100ModbusMaster*       m_modbus_computer;

signals:
    void showErrorDialog(QString);

    void UpdateDeviceInfo(QString); // Отпраяем требование изменить данные о установке

    void UpdateStatus(QString, int); // Обновление статуса (Статус и цвет)
    void SendErrorInfo(QString); // вывод сообщения об ошибке

    void UpdateData(InputDataTypeDef); // отправляем указатель на новые данные... ? надо ли указатель или задать его статически...

public slots:
    void poller(void); // обновление данных для чтения

// начало соединения
        void m_connect(int port,
                     int bauds,
                     int stop_bits,
                     int data_bits,
                     QString parity,
                     int response_time,
                     int modbus_net_addr);

    void start(    uint8_t Mode,             // тип напряжения 1-AC, 0-DC, 2-Прожиг
                   uint16_t U,                 // напряжение испытания
                   uint16_t I,                 // Максимальный ток при котором происходит отключение
                   uint16_t U_Speed,           // Скорость поднятия напряжения
                   uint16_t timer_min,        // Время испытания минуты
                   uint8_t time_dir,          // Направление отсчета: 0 прямое,  1 - обратное
                   uint8_t off_after_timer,   // После обратного отсчета: 0 - звуковой сигнал, 1 - завершение испытания
                   uint8_t time_start        // Запуск таймера при прямом отсчете: 0 - кнопкой, 1 - авто
            ); // Включение высокого напряжения

    void stop();  // Отключение высокого напряжения

    void SendSteps(int steps); // Отправка настроек режима испытания в установку

 //   void ChangeVolage (quint16 Voltage); // Изменение напряжения в процессе подачи напряжения


private:
    void syncSettings(); // Синхронизация настроек установки и программы (если это необходимо)

    void loadPreSets();  // Чтение предустановленных программ испытания из файла настроек

    void SevePreSets();  // Запись предустановленных программ испытания в файл настроек

private:


// Статуы работы высоковольтной установки (регистр "mode")
#define MANUAL_MODE             0
#define AUTO_MODE               1
#define PROGRAMM_MODE           2
#define BURN_MODE               3

#define MENU_MODE               4
#define SETUP_MODE              5
#define NAME_MODE               6
#define EDIT_MODE               7
#define ARHIVE_MODE             8
#define CALIBR_MODE             10
#define BOOT_MODE               12

// Разбор ошибок, констатны
// Разбор ошибок.
#define DOOR_ERROR              (1<<0)  /// открыта дверь при использовании внешней сигнализации
#define UART_ERROR              (1<<1)  /// ошибка связи с платой ВВ блока
#define LATR_ERROR              (1<<2)  /// ЛАТР не становится в ноль
#define LATR_OFF_ERROR          (1<<3)  /// нет напряжения с ЛАТРа
#define PROBOY_ERROR            (1<<4)  /// произошел пробой в нагрузке
#define DEAD_ERROR              (1<<5)  /// подозрение на межвитковое замыкание
#define UART_ERROR_CONST        (1<<6)  /// ошибка связи с платой ВВ блока при чтении калибровок
#define RESTART_ERROR           (1<<7)  /// производится перезагрузка платы ВВ блока
#define DIODE_ERROR             (1<<8)  /// пробит диод или не работает коммутатор (большой переменный ток в режиме постоянного напряжения)
#define OVERHEAT_ERROR          (1<<9)  /// перегрев трансформатора
#define LOW_HV_ERROR            (1<<10) /// ошибка подключения первички к латру (или делителя к выходу тра-ра)
#define OVERVOLTAGE_ERROR       (1<<11) /// напряжение на выходе подозрительно больше максимального (например латр не туда поехал)
#define DC_ON_HV_ERROR          (1<<12) /// при испытании AC на выходе много DC (диод на выходе)
#define LATR_WRONG_DIR_ERROR    (1<<13) /// неправильное направление вращения ШД



    TCoreModes          CoreModes;

    QTimer*             m_polling_timer;



};





#endif // CORE_H
