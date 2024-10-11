#include "device_v100.h"
#include "ui_device_v100.h"

#include "stingraygui.h"
#include "ui_device_v100.h"
#include <QSettings>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDir>
#include <QSettings>
#include <QMessageBox>

#include "v100_core.h"

QSettings m_settings (QDir::currentPath() + "/config.ini", QSettings::IniFormat);

QSharedPointer<V100Core>                m_core;

StingrayGUI::StingrayGUI(QWidget *parent)
    : Device(parent)
    , ui(new Ui::Device_V100)
{
    m_core = QSharedPointer<V100Core>::create();

    ui->setupUi(this);
    on_ScanPortButton_clicked();
    ui->StopButton->setStyleSheet("QPushButton { background-color : darkRed; color : black;}");
    ui->StartButton->setStyleSheet("QPushButton { background-color : darkGreen; color : black;}");
    ui->ProgrammControl->setVisible(false);
    ui->AutoControl->setVisible(false);

    // Чтение настроек из файла
    on_ReadButton_clicked();
    ui->ProgrammSelectBox->clear();
    for (int i=0; i<10; i++)
    {
        ui->ProgrammSelectBox->addItem(PreSets[i].name);
    }
    ui->tabWidget->setCurrentIndex(0);

    palette.setColor(QPalette::Base, Qt::darkGray);
    palette.setColor(QPalette::Text, Qt::black);
    ui->StatusLine->setPalette(palette);
    ui->StartButton->setEnabled(false);
    //ui->StopButton->setEnabled(false);
    ui->label_31->setVisible(false);

    QObject::connect(m_core.data() , SIGNAL(UpdateDeviceInfo(QString)),this,
                     SLOT(ShowDeviceInfo(QString)));

    QObject::connect(ui->connectButton , &QAbstractButton::clicked, [this]{
       ui->StartButton->setEnabled(false);
       //m_core->m_connect(12,19200,2,8,"No party", 250, 3);
       m_core->m_connect(ui->PortBox->currentText().remove(0,3).toInt(),ui->BoudReteBox->currentText().toInt(), 2,8, "No parity", 250, ui->IDEdit->text().toInt());
    });

//    QObject::connect(m_core.data() , SIGNAL(UpdateStatus(QString, int)),this,
 //                    SLOT(ShowStatus(QString info,int color)));

    QObject::connect(m_core.data() , SIGNAL(showErrorDialog(QString)),this,
                     SLOT(ShowErrorInfo(QString)));

    QObject::connect(m_core.data() , SIGNAL(UpdateData(V100Core::InputDataTypeDef)),this,
                     SLOT(ProcessReadedData(V100Core::InputDataTypeDef)));

}

StingrayGUI::~StingrayGUI()
{
    delete ui;
}

void StingrayGUI::on_ScanPortButton_clicked()
{
    ui->PortBox->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
    {
        QStringList list;
        list << info.portName();
        ui->PortBox->addItem(list.first());
    }
    if( ui->PortBox->count()>0)
    {
        ui->connectButton->setEnabled(true);
    }
}


//void StingrayGUI::on_connectButton_clicked()
//{
//    m_core->connect(12,19200,2,8,"No party", 250, 3);
 //   //connect(ui->PortBox->currentText().remove(0,3).toInt(),ui->BoudReteBox->currentText().toInt(), 2,8, "No parity", 250, 3);
//}


void StingrayGUI::on_ModeBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
    case 1:
        {
            ui->ManualControl->setVisible(true);
            ui->ProgrammControl->setVisible(false);
            ui->AutoControl->setVisible(false);
        }break;

    case 2:
    case 3:
    case 4:
        {
            ui->ManualControl->setVisible(false);
            ui->ProgrammControl->setVisible(false);
            ui->AutoControl->setVisible(true);
        }break;

    case 5:
        {
            ui->ManualControl->setVisible(false);
            ui->ProgrammControl->setVisible(true);
            ui->AutoControl->setVisible(false);
        }break;

 /*   default:
            {
                ui->ManualControl->setVisible(false);
                ui->ProgrammControl->setVisible(false);
                ui->AutoControl->setVisible(false);
            }
        break;*/
    }
}


void StingrayGUI::on_TimerPreSetBox_activated(int index)
{
    switch (index) {
    case 0:// 1min
    {
        ui->TimerSecValue->setValue(60*1);
    }break;

    case 1:// 1min
    {
        ui->TimerSecValue->setValue(60*5);
    }break;

    case 2:// 1min
    {
        ui->TimerSecValue->setValue(60*10);
    }break;

    case 3:// 1min
    {
        ui->TimerSecValue->setValue(60*15);
    }break;

    default:
    {
        ui->TimerSecValue->setValue(10);
    }break;
    }
}


void StingrayGUI::on_AutoVoltageSlider_valueChanged(int value)
{
    ui->AutoVoltageValue->setValue(value);
}


void StingrayGUI::on_AutoVoltageValue_valueChanged(int arg1)
{
    ui->AutoVoltageSlider->setValue(arg1);
}


void StingrayGUI::on_ReadButton_clicked()
{
    QStringList sections = m_settings.childGroups();

if ( sections.empty() )
    {// Если список пустой то записываем его по умолчанию
        m_settings.setValue("AC1/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("AC1/AC", 0);
        m_settings.setValue("AC1/U", 100);
        m_settings.setValue("AC1/I", 200);
        m_settings.setValue("AC1/U_Speed", 2);
        m_settings.setValue("AC1/U_time_min", 0);
        m_settings.setValue("AC1/U_time_sec", 60);

        m_settings.setValue("AC2/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("AC2/AC", 0);
        m_settings.setValue("AC2/U", 100);
        m_settings.setValue("AC2/I", 200);
        m_settings.setValue("AC2/U_Speed", 2);
        m_settings.setValue("AC2/U_time_min", 0);
        m_settings.setValue("AC2/U_time_sec", 60);

        m_settings.setValue("AC3/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("AC3/AC", 0);
        m_settings.setValue("AC3/U", 100);
        m_settings.setValue("AC3/I", 200);
        m_settings.setValue("AC3/U_Speed", 2);
        m_settings.setValue("AC3/U_time_min", 0);
        m_settings.setValue("AC3/U_time_sec", 60);

        m_settings.setValue("AC4/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("AC4/AC", 0);
        m_settings.setValue("AC4/U", 100);
        m_settings.setValue("AC4/I", 200);
        m_settings.setValue("AC4/U_Speed", 2);
        m_settings.setValue("AC4/U_time_min", 0);
        m_settings.setValue("AC4/U_time_sec", 60);

        m_settings.setValue("AC5/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("AC5/AC", 0);
        m_settings.setValue("AC5/U", 100);
        m_settings.setValue("AC5/I", 200);
        m_settings.setValue("AC5/U_Speed", 2);
        m_settings.setValue("AC5/U_time_min", 0);
        m_settings.setValue("AC5/U_time_sec", 60);

        m_settings.setValue("DC1/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("DC1/AC", 1);
        m_settings.setValue("DC1/U", 100);
        m_settings.setValue("DC1/I", 200);
        m_settings.setValue("DC1/U_Speed", 2);
        m_settings.setValue("DC1/U_time_min", 0);
        m_settings.setValue("DC1/U_time_sec", 60);

        m_settings.setValue("DC2/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("DC2/AC", 1);
        m_settings.setValue("DC2/U", 100);
        m_settings.setValue("DC2/I", 200);
        m_settings.setValue("DC2/U_Speed", 2);
        m_settings.setValue("DC2/U_time_min", 0);
        m_settings.setValue("DC2/U_time_sec", 60);

        m_settings.setValue("DC3/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("DC3/AC", 1);
        m_settings.setValue("DC3/U", 100);
        m_settings.setValue("DC3/I", 200);
        m_settings.setValue("DC3/U_Speed", 2);
        m_settings.setValue("DC3/U_time_min", 0);
        m_settings.setValue("DC3/U_time_sec", 60);

        m_settings.setValue("DC4/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("DC4/AC", 1);
        m_settings.setValue("DC4/U", 100);
        m_settings.setValue("DC4/I", 200);
        m_settings.setValue("DC4/U_Speed", 2);
        m_settings.setValue("DC4/U_time_min", 0);
        m_settings.setValue("DC4/U_time_sec", 60);

        m_settings.setValue("DC5/Name", "Испытание резинотехнического изделия №2");
        m_settings.setValue("DC5/AC", 1);
        m_settings.setValue("DC5/U", 100);
        m_settings.setValue("DC5/I", 200);
        m_settings.setValue("DC5/U_Speed", 2);
        m_settings.setValue("DC5/U_time_min", 0);
        m_settings.setValue("DC5/U_time_sec", 60);
        m_settings.sync();
} else{ // если нет то читаем
        int i=0;
        for (const auto &item : sections)
        {
            PreSets[i].name = m_settings.value(QString("%1/name").arg(item)).toString();
            PreSets[i].Preset.AC_DC         = m_settings.value(QString("%1/AC").arg(item)).toInt();
            PreSets[i].Preset.U             = m_settings.value(QString("%1/U").arg(item)).toInt();
            PreSets[i].Preset.I             = m_settings.value(QString("%1/I").arg(item)).toInt();
            PreSets[i].Preset.U_Speed       = m_settings.value(QString("%1/U_Speed").arg(item)).toInt();
            PreSets[i].Preset.timer_min    = m_settings.value(QString("%1/U_time_min").arg(item)).toInt();
            PreSets[i].Preset.off_after_timer= 1;
            PreSets[i].Preset.time_dir      = 1;
            PreSets[i].Preset.time_start    = 1;
            i++;
        };
    }

ui->AC1NameEdit->setText(PreSets[0].name);
ui->AC1VoltageBox->setValue(PreSets[0].Preset.U);
ui->AC1CurrentBox->setValue(PreSets[0].Preset.I);
ui->AC1AutoSpeedBox->setCurrentIndex(PreSets[0].Preset.U_Speed);
ui->AC1Time->setValue(PreSets[0].Preset.timer_min);

ui->AC2NameEdit->setText(PreSets[1].name);
ui->AC2VoltageBox->setValue(PreSets[1].Preset.U);
ui->AC2CurrentBox->setValue(PreSets[1].Preset.I);
ui->AC2AutoSpeedBox->setCurrentIndex(PreSets[1].Preset.U_Speed);
ui->AC2Time->setValue(PreSets[1].Preset.timer_min);

ui->AC3NameEdit->setText(PreSets[2].name);
ui->AC3VoltageBox->setValue(PreSets[2].Preset.U);
ui->AC3CurrentBox->setValue(PreSets[2].Preset.I);
ui->AC3AutoSpeedBox->setCurrentIndex(PreSets[2].Preset.U_Speed);
ui->AC3Time->setValue(PreSets[2].Preset.timer_min);

ui->AC4NameEdit->setText(PreSets[3].name);
ui->AC4VoltageBox->setValue(PreSets[3].Preset.U);
ui->AC4CurrentBox->setValue(PreSets[3].Preset.I);
ui->AC4AutoSpeedBox->setCurrentIndex(PreSets[3].Preset.U_Speed);
ui->AC4Time->setValue(PreSets[3].Preset.timer_min);

ui->AC5NameEdit->setText(PreSets[4].name);
ui->AC5VoltageBox->setValue(PreSets[4].Preset.U);
ui->AC5CurrentBox->setValue(PreSets[4].Preset.I);
ui->AC5AutoSpeedBox->setCurrentIndex(PreSets[4].Preset.U_Speed);
ui->AC5Time->setValue(PreSets[4].Preset.timer_min);

ui->DC1NameEdit->setText(PreSets[5].name);
ui->DC1VoltageBox->setValue(PreSets[5].Preset.U);
ui->DC1CurrentBox->setValue(PreSets[5].Preset.I);
ui->DC1AutoSpeedBox->setCurrentIndex(PreSets[5].Preset.U_Speed);
ui->DC1Time->setValue(PreSets[5].Preset.timer_min);

ui->DC2NameEdit->setText(PreSets[6].name);
ui->DC2VoltageBox->setValue(PreSets[6].Preset.U);
ui->DC2CurrentBox->setValue(PreSets[6].Preset.I);
ui->DC2AutoSpeedBox->setCurrentIndex(PreSets[6].Preset.U_Speed);
ui->DC2Time->setValue(PreSets[6].Preset.timer_min);

ui->DC3NameEdit->setText(PreSets[7].name);
ui->DC3VoltageBox->setValue(PreSets[7].Preset.U);
ui->DC3CurrentBox->setValue(PreSets[7].Preset.I);
ui->DC3AutoSpeedBox->setCurrentIndex(PreSets[7].Preset.U_Speed);
ui->DC3Time->setValue(PreSets[7].Preset.timer_min);

ui->DC4NameEdit->setText(PreSets[8].name);
ui->DC4VoltageBox->setValue(PreSets[8].Preset.U);
ui->DC4CurrentBox->setValue(PreSets[8].Preset.I);
ui->DC4AutoSpeedBox->setCurrentIndex(PreSets[8].Preset.U_Speed);
ui->DC4Time->setValue(PreSets[8].Preset.timer_min);

ui->DC5NameEdit->setText(PreSets[9].name);
ui->DC5VoltageBox->setValue(PreSets[9].Preset.U);
ui->DC5CurrentBox->setValue(PreSets[9].Preset.I);
ui->DC5AutoSpeedBox->setCurrentIndex(PreSets[9].Preset.U_Speed);
ui->DC5Time->setValue(PreSets[9].Preset.timer_min);
}

void StingrayGUI::on_WriteButton_clicked()
{


    m_settings.setValue("AC1/Name", ui->AC1NameEdit->text());
    m_settings.setValue("AC1/U", ui->AC1VoltageBox->value());
    m_settings.setValue("AC1/I", ui->AC1CurrentBox->value());
    m_settings.setValue("AC1/U_Speed", ui->AC1AutoSpeedBox->currentIndex());
    m_settings.setValue("AC1/U_time_min", ui->AC1Time->value());
    m_settings.setValue("AC1/U_time_sec", 0);

    m_settings.setValue("AC2/Name", ui->AC2NameEdit->text());
    m_settings.setValue("AC2/U", ui->AC2VoltageBox->value());
    m_settings.setValue("AC2/I", ui->AC2CurrentBox->value());
    m_settings.setValue("AC2/U_Speed", ui->AC2AutoSpeedBox->currentIndex());
    m_settings.setValue("AC2/U_time_min", ui->AC2Time->value());
    m_settings.setValue("AC2/U_time_sec", 0);

    m_settings.setValue("AC3/Name", ui->AC3NameEdit->text());
    m_settings.setValue("AC3/U", ui->AC3VoltageBox->value());
    m_settings.setValue("AC3/I", ui->AC3CurrentBox->value());
    m_settings.setValue("AC3/U_Speed", ui->AC3AutoSpeedBox->currentIndex());
    m_settings.setValue("AC3/U_time_min", ui->AC3Time->value());
    m_settings.setValue("AC3/U_time_sec", 0);

    m_settings.setValue("AC4/Name", ui->AC4NameEdit->text());
    m_settings.setValue("AC4/U", ui->AC4VoltageBox->value());
    m_settings.setValue("AC4/I", ui->AC4CurrentBox->value());
    m_settings.setValue("AC4/U_Speed", ui->AC4AutoSpeedBox->currentIndex());
    m_settings.setValue("AC4/U_time_min", ui->AC4Time->value());
    m_settings.setValue("AC4/U_time_sec", 0);

    m_settings.setValue("AC5/Name", ui->AC5NameEdit->text());
    m_settings.setValue("AC5/U", ui->AC5VoltageBox->value());
    m_settings.setValue("AC5/I", ui->AC5CurrentBox->value());
    m_settings.setValue("AC5/U_Speed", ui->AC5AutoSpeedBox->currentIndex());
    m_settings.setValue("AC5/U_time_min", ui->AC5Time->value());
    m_settings.setValue("AC5/U_time_sec", 0);

    m_settings.setValue("DC1/Name", ui->DC1NameEdit->text());
    m_settings.setValue("DC1/U", ui->DC1VoltageBox->value());
    m_settings.setValue("DC1/I", ui->DC1CurrentBox->value());
    m_settings.setValue("DC1/U_Speed", ui->DC1AutoSpeedBox->currentIndex());
    m_settings.setValue("DC1/U_time_min", ui->DC1Time->value());
    m_settings.setValue("DC1/U_time_sec", 0);

    m_settings.setValue("DC2/Name", ui->DC2NameEdit->text());
    m_settings.setValue("DC2/U", ui->DC2VoltageBox->value());
    m_settings.setValue("DC2/I", ui->DC2CurrentBox->value());
    m_settings.setValue("DC2/U_Speed", ui->DC2AutoSpeedBox->currentIndex());
    m_settings.setValue("DC2/U_time_min", ui->DC2Time->value());
    m_settings.setValue("DC2/U_time_sec", 0);

    m_settings.setValue("DC3/Name", ui->DC3NameEdit->text());
    m_settings.setValue("DC3/U", ui->DC3VoltageBox->value());
    m_settings.setValue("DC3/I", ui->DC3CurrentBox->value());
    m_settings.setValue("DC3/U_Speed", ui->DC3AutoSpeedBox->currentIndex());
    m_settings.setValue("DC3/U_time_min", ui->DC3Time->value());
    m_settings.setValue("DC3/U_time_sec", 0);

    m_settings.setValue("DC4/Name", ui->DC4NameEdit->text());
    m_settings.setValue("DC4/U", ui->DC4VoltageBox->value());
    m_settings.setValue("DC4/I", ui->DC4CurrentBox->value());
    m_settings.setValue("DC4/U_Speed", ui->DC4AutoSpeedBox->currentIndex());
    m_settings.setValue("DC4/U_time_min", ui->DC4Time->value());
    m_settings.setValue("DC4/U_time_sec", 0);

    m_settings.setValue("DC5/Name", ui->DC5NameEdit->text());
    m_settings.setValue("DC5/U", ui->DC5VoltageBox->value());
    m_settings.setValue("DC5/I", ui->DC5CurrentBox->value());
    m_settings.setValue("DC5/U_Speed", ui->DC5AutoSpeedBox->currentIndex());
    m_settings.setValue("DC5/U_time_min", ui->DC5Time->value());
    m_settings.setValue("DC5/U_time_sec", 0);
}


void StingrayGUI::on_ProgrammSelectBox_currentIndexChanged(int index)
{// Выбор программы испытания и обновление массива данных для отправки из полей ввода для того чтобы применялись изменения без сохранения
    switch (index) {
    case 0:
    {
        PreSets[index].name = ui->AC1NameEdit->text();
        PreSets[index].Preset.U = ui->AC1VoltageBox->value();
        PreSets[index].Preset.I = ui->AC1CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->AC1AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->AC1Time->value();

        ui->ProgrammTypeLine->setText("AC");
        ui->ProgrammVoltageLine->setText(ui->AC1VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->AC1CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->AC1AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->AC1Time->text());
    }   break;

    case 1:
    {
        PreSets[index].name = ui->AC2NameEdit->text();
        PreSets[index].Preset.U = ui->AC2VoltageBox->value();
        PreSets[index].Preset.I = ui->AC2CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->AC2AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->AC2Time->value();

        ui->ProgrammTypeLine->setText("AC");
        ui->ProgrammVoltageLine->setText(ui->AC2VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->AC2CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->AC2AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->AC2Time->text());
    }   break;

    case 2:
    {
        PreSets[index].name = ui->AC3NameEdit->text();
        PreSets[index].Preset.U = ui->AC3VoltageBox->value();
        PreSets[index].Preset.I = ui->AC3CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->AC3AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->AC3Time->value();

        ui->ProgrammTypeLine->setText("AC");
        ui->ProgrammVoltageLine->setText(ui->AC3VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->AC3CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->AC3AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->AC3Time->text());
    }   break;

    case 3:
    {
        PreSets[index].name = ui->AC4NameEdit->text();
        PreSets[index].Preset.U = ui->AC4VoltageBox->value();
        PreSets[index].Preset.I = ui->AC4CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->AC4AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->AC4Time->value();

        ui->ProgrammTypeLine->setText("AC");
        ui->ProgrammVoltageLine->setText(ui->AC4VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->AC4CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->AC4AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->AC4Time->text());
    }   break;

    case 4:
    {
        PreSets[index].name = ui->AC5NameEdit->text();
        PreSets[index].Preset.U = ui->AC5VoltageBox->value();
        PreSets[index].Preset.I = ui->AC5CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->AC5AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->AC5Time->value();

        ui->ProgrammTypeLine->setText("AC");
        ui->ProgrammVoltageLine->setText(ui->AC5VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->AC5CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->AC5AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->AC5Time->text());
    }   break;

    case 5:
    {
        PreSets[index].name = ui->DC1NameEdit->text();
        PreSets[index].Preset.U = ui->DC1VoltageBox->value();
        PreSets[index].Preset.I = ui->DC1CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->DC1AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->DC1Time->value();

        ui->ProgrammTypeLine->setText("DC");
        ui->ProgrammVoltageLine->setText(ui->DC1VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->DC1CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->DC1AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->DC1Time->text());
    }   break;

    case 6:
    {
        PreSets[index].name = ui->DC2NameEdit->text();
        PreSets[index].Preset.U = ui->DC2VoltageBox->value();
        PreSets[index].Preset.I = ui->DC2CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->DC2AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->DC2Time->value();

        ui->ProgrammTypeLine->setText("DC");
        ui->ProgrammVoltageLine->setText(ui->DC2VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->DC2CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->DC2AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->DC2Time->text());
    }   break;

    case 7:
    {
        PreSets[index].name = ui->DC3NameEdit->text();
        PreSets[index].Preset.U = ui->DC3VoltageBox->value();
        PreSets[index].Preset.I = ui->DC2CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->DC3AutoSpeedBox->currentIndex();
        PreSets[index].Preset.timer_min = ui->DC3Time->value();

        ui->ProgrammTypeLine->setText("DC");
        ui->ProgrammVoltageLine->setText(ui->DC3VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->DC3CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->DC3AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->DC3Time->text());
    }   break;

    case 8:
    {
        PreSets[index].name = ui->DC4NameEdit->text();
        PreSets[index].Preset.U = ui->DC4VoltageBox->value();
        PreSets[index].Preset.I = ui->DC4CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->DC4AutoSpeedBox->currentIndex();
  //      PreSets[index].Preset.U_time_min = ui->DC4Time->value();

        ui->ProgrammTypeLine->setText("DC");
        ui->ProgrammVoltageLine->setText(ui->DC4VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->DC4CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->DC4AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->DC4Time->text());
    }   break;

    case 9:
    {
        PreSets[index].name = ui->DC5NameEdit->text();
        PreSets[index].Preset.U = ui->DC5VoltageBox->value();
        PreSets[index].Preset.I = ui->DC5CurrentBox->value();
        PreSets[index].Preset.U_Speed = ui->DC5AutoSpeedBox->currentIndex();
    //    PreSets[index].Preset.U_time_min = ui->DC5Time->value();

        ui->ProgrammTypeLine->setText("DC");
        ui->ProgrammVoltageLine->setText(ui->DC5VoltageBox->text());
        ui->ProgrammCurrentLine->setText(ui->DC5CurrentBox->text());
        ui->ProgrammSPEdit->setText(ui->DC5AutoSpeedBox->currentText());
        ui->ProgrammTimeLine->setText(ui->DC5Time->text());
    }   break;

    default:
        break;
    }

}

//
// void StingrayGUI::on_PresetNameCanged (int index)
// { // обновляем названия программ испытания в перечне
//     QString itsName = "AC";
//     itsName.append(QString::number(index+1));
//     itsName.append("NameEdit");
//
//     QLineEdit *findObj = this->findChild<QLineEdit *>(itsName);// Поиск объекта
//
//     ui->ProgrammSelectBox->setItemText(index, findObj->text());
// }

void StingrayGUI::on_AC1NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(0, arg1);
}


void StingrayGUI::on_tabWidget_currentChanged(int index)
{
    if (index == 0)
    {
        int t = ui->ProgrammSelectBox->currentIndex();
        ui->ProgrammSelectBox->currentIndexChanged(t);
    }
}


void StingrayGUI::on_AC2NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(1, arg1);
}


void StingrayGUI::on_AC3NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(2, arg1);
}


void StingrayGUI::on_AC4NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(3, arg1);
}


void StingrayGUI::on_AC5NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(4, arg1);
}


void StingrayGUI::on_DC1NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(5, arg1);
}


void StingrayGUI::on_DC2NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(6, arg1);
}


void StingrayGUI::on_DC3NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(7, arg1);
}


void StingrayGUI::on_DC4NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(8, arg1);
}


void StingrayGUI::on_DC5NameEdit_textChanged(const QString &arg1)
{
    ui->ProgrammSelectBox->setItemText(9, arg1);
}

/*
 *
#define QTY_DATA_REG 10;

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
 *
 */

void StingrayGUI::ProcessReadedData (V100Core::InputDataTypeDef data)
{// Отображение данных, прочитанных из установки
 //
//
    uint16_t b = 0;
    QString buf;
    // Далее идёт разбор данных куда что запихнуть для индикации
    b = data.U_max%100;
    if (b<10) { buf='0'+QString::number(b);} else{ buf=QString::number(b);}
    ui->UampLine->setText(QString::number(data.U_max/100)+"."+buf); // Вроде это так должно работать
    b = data.U_rms%100;
    if (b<10) { buf='0'+QString::number(b);} else{ buf=QString::number(b);}
    ui->UrmsLine->setText(QString::number(data.U_rms/100)+"."+buf);
    b = data.I_out%100;
    if (b<10) { buf='0'+QString::number(b);} else{ buf=QString::number(b);}
    ui->IrmsLine->setText(QString::number(data.I_out/100)+"."+buf);
    ui->I10secLine->setText(QString::number(data.I_leakage));//100)+"."+QString::number(data.I_leakage%100));

    ui->PowerLine->setText(QString::number(data.P_out));

    if (!(data.status==0)) {
        ui->TimerMinValue->setReadOnly(true);
        ui->TimerSecValue->setReadOnly(true);
        ui->TimerMinValue->setValue(data.time_in_seconds/60);
        ui->TimerSecValue->setValue(data.time_in_seconds%60);
        // обновление таймера из установки только тогда когда включено высокое напряжение чтобы шёл обратный отсчёт
        ui->label_31->setVisible(true);
    }
    else {ui->label_31->setVisible(false);}
    // параметры     T1_REG_NO, T2_REG_NO пока не выводим
}

void StingrayGUI::ShowDeviceInfo(QString deviceInfo) // Отпраяем требование изменить данные о установке
{
    ui->ModelLine->setText(deviceInfo.toUtf8());
    ui->StartButton->setStyleSheet("QPushButton { background-color : green; color : black;}");
    ui->StartButton->setEnabled(true);
    ui->StatusLine->setText("Подключено");
}

void StingrayGUI::ShowErrorInfo(QString info)// Показываем сообщение об ошибке
{
    //ui->statusbar->showMessage(info,3000);
    QMessageBox::information(this, "Ошибка", info);
}

void StingrayGUI::ShowStatus(QString info,int color) // Показываем состояние
{
   // QPalette palette;

    ui->StatusLine->setText("Латр, привод");

   switch (color) {
   case 0:
   {
       palette.setColor(QPalette::Base, Qt::gray);
       palette.setColor(QPalette::Text, Qt::black);
   }break;

   case 1:
   {
       palette.setColor(QPalette::Base, Qt::black);
       palette.setColor(QPalette::Text, Qt::white);
   }break;

   case 2:
   {
       palette.setColor(QPalette::Base, Qt::darkRed);
       palette.setColor(QPalette::Text, Qt::white);
   }break;

   case 3:
   {
       palette.setColor(QPalette::Base, Qt::darkGreen);
       palette.setColor(QPalette::Text, Qt::white);
   }break;

   case 4:
   {
       palette.setColor(QPalette::Base, Qt::green);
       palette.setColor(QPalette::Text, Qt::black);
   }break;

   default:
       break;
   }

   ui->StatusLine->setPalette(palette);
}

QVector<QStringList> StingrayGUI::protocol()
{
    return Device::protocol();
}

void StingrayGUI::on_StartButton_clicked()
{
    uint8_t time_dir = 0;
    uint8_t off_after_timer = 0;
    //uint8_t time_star = 1; // автозапуск начала отсчёта при прямом отсчёте запуск таймера - 1, 0 - в ручную, по нажатию пуск
    uint16_t U_Speed = 0;

    if (ui->radioButtonDown->isChecked())
    {//  направление - 0 Вверх 1- обратный
        time_dir = 1;
    }

    if(ui->TimeActionBox->currentIndex()==0)
    { // авто отключение после обратного отсчёта 1 -выключаем 0- звук
        off_after_timer = 1;
    }
    else{off_after_timer = 0;}

    switch (ui->AutoSpeedBox->currentIndex()) {
    case 0:
        {U_Speed = 50;}break;
    case 1:
        {U_Speed = 100;}break;
    case 2:
        {U_Speed = 200;}break;
    default:
        break;
    }

    switch (ui->ModeBox->currentIndex()) {
    case 0: //2-AC-ручное
    {
        m_core->start(2,ui->ManVBox->value()*100,ui->ManIBox->value()*100,1,ui->TimerMinValue->value(), time_dir, off_after_timer, 1);
    }break;

    case 1: //0-DC-ручное
    {
        m_core->start(0,ui->ManVBox->value()*100,ui->ManIBox->value()*100,1,ui->TimerMinValue->value(), time_dir, off_after_timer, 1);
    }break;

    case 2: //3-АC-авто
    {

        m_core->start(3,ui->AutoVoltageValue->value()*100,ui->AutoCurValue->value()*100,U_Speed,ui->TimerMinValue->value(), time_dir, off_after_timer, 1);
    }break;

    case 3: //1-DC-авто
    {

        m_core->start(1,ui->AutoVoltageValue->value()*100,ui->AutoCurValue->value()*100,U_Speed,ui->TimerMinValue->value(), time_dir, off_after_timer, 1);
    }break;

    case 4: //4-Прожиг
    {

        m_core->start(4,ui->AutoVoltageValue->value()*100,ui->AutoCurValue->value()*100,U_Speed,ui->TimerMinValue->value()*100, time_dir, off_after_timer, 1);
    }break;

    case 5: //4-программа
    {
        switch (PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.U_Speed) {
        case 0:
        {U_Speed = 50;}break;
        case 1:
        {U_Speed = 100;}break;
        case 2:
        {U_Speed = 200;}break;
        default:
            break;
        }
        if (ui->ProgrammSelectBox->currentIndex()<5)
        {//3-АC-авто
            m_core->start(3,PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.U*100,PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.I*100,U_Speed,PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.timer_min, time_dir, off_after_timer, 1);
        }
        else
        {//1-DC-авто
            m_core->start(1,PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.U*100,PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.I*100,U_Speed,PreSets[ui->ProgrammSelectBox->currentIndex()].Preset.timer_min, time_dir, off_after_timer, 1);
        }

    }break;

    default:
        break;
    }
}


void StingrayGUI::on_StopButton_clicked()
{
    m_core->stop();
    ui->TimerMinValue->setReadOnly(false);
    ui->TimerSecValue->setReadOnly(false);
    ui->TimerMinValue->setValue(1);
    ui->TimerSecValue->setValue(0);
}


void StingrayGUI::on_ManualUP_clicked()
{
    m_core->SendSteps(10);
}


void StingrayGUI::on_ManualDown_clicked()
{
    m_core->SendSteps(-10);
}


void StingrayGUI::on_ManualUP_2_clicked()
{
    m_core->SendSteps(1);
}


void StingrayGUI::on_ManualDown_2_clicked()
{
    m_core->SendSteps(-1);
}








