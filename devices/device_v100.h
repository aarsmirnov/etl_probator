#ifndef STINGRAYGUI_H
#define STINGRAYGUI_H

#include <QMainWindow>
#include "v100_core.h"
#include <QVector>


/* Defining */
#define ORGANIZATION_NAME "Energoscan"
#define ORGANIZATION_DOMAIN "www.Energoskan.ru"
#define APPLICATION_NAME "StingRayGUI"

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


enum STATUSCODES
{
    NO_CONNECT_S    = 0, // Нет связи
    NORMAL_S        = 1, // Нормальная работа, режим ожидания
    EXT_BLOCK_S     = 2, // Разорвана внешняя блокировка
    DC_MAN_RUN_S    = 17,// Включено постоянное напряжение в ручную
    AC_MAN_RUN_S    = 18,// Включено переменное напряжение в ручную
    DC_AUT_RUN_S    = 19,// Включено постоянное напряжение авто
    AC_AUT_RUN_S    = 20,// Включено переменное напряжение авто
    WAIT_FOR_DISCH_S= 21,// Ждём разряда после завершения испытания
    END_TEST_S      = 22,// Закончили испытание нет пробоя
    END_BREAK_S     = 23,// Закончили испытание пробоем
    DC_PRG_RUN_S    = 24,// По программе включено
    AC_PRG_RUN_S    = 25,// По программе включено
    BURN_RUN_S      = 33,// Работаетм в режиме прожига

    COMM_ERR_S      = 0xEE01,// Нет связи с измерительной частью
    LATR_ERR_S      = 0xEE02,// Нет начальной проверки латра
    LATR_NO_O_ERR_S = 0xEE03 // Нет возврата латра в 0
};

class Core;

QT_BEGIN_NAMESPACE
namespace Ui {
class Device_V100;
}
QT_END_NAMESPACE

#include "device.h"

class Device_V100 : public Device
{
    Q_OBJECT

public:
   explicit Device_V100(const QString &title, const QPixmap &schema, Device *parent = nullptr);
    ~Device_V100();

public slots:
    void ProcessReadedData(V100Core::InputDataTypeDef data); // Показываем включили или не установку
    void ShowDeviceInfo(QString deviceInfo); // Отпраяем требование изменить данные о установке

    void ShowErrorInfo(QString info);// Показываем сообщение об ошибке
    void ShowStatus(QString info,int color); // Показываем состояние

    QString name() const override { return QStringLiteral("Высоковольтные испытания ВИУ-100-70"); }
    QVector<QStringList> protocol() override;

private slots:
    void on_ScanPortButton_clicked();

 //   void on_connectButton_clicked();

    void on_ModeBox_currentIndexChanged(int index);

    void on_TimerPreSetBox_activated(int index);

    void on_AutoVoltageSlider_valueChanged(int value);

    void on_AutoVoltageValue_valueChanged(int arg1);

    void on_ReadButton_clicked();

    void on_WriteButton_clicked();

    void on_ProgrammSelectBox_currentIndexChanged(int index);

    void on_AC1NameEdit_textChanged(const QString &arg1);

    void on_tabWidget_currentChanged(int index);

    void on_AC2NameEdit_textChanged(const QString &arg1);

    void on_AC3NameEdit_textChanged(const QString &arg1);

    void on_AC4NameEdit_textChanged(const QString &arg1);

    void on_AC5NameEdit_textChanged(const QString &arg1);

    void on_DC1NameEdit_textChanged(const QString &arg1);

    void on_DC2NameEdit_textChanged(const QString &arg1);

    void on_DC3NameEdit_textChanged(const QString &arg1);

    void on_DC4NameEdit_textChanged(const QString &arg1);

    void on_DC5NameEdit_textChanged(const QString &arg1);


    void on_StartButton_clicked();

    void on_StopButton_clicked();

    void on_ManualUP_clicked();

    void on_ManualDown_clicked();

    void on_ManualUP_2_clicked();

    void on_ManualDown_2_clicked();



private:
    Ui::Device_V100 *ui;

    typedef struct // Структура предустановки программы испытания
    {
        //char name[36]; // это в контроллере но это отправлять туда не будем
        QString name; // только для индикации в программе
        V100Core::SetTypeDef Preset;

    } MemorySlotTypeDef;

    MemorySlotTypeDef PreSets[10];

    QPalette palette;

    };



#endif // STINGRAYGUI_H
