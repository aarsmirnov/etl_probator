#include "device_iks30a.h"
#include "ui_device_iks30a.h"

#include "utils.h"

#include <QDebug>

namespace {
    const QString kStylesheetPath(":/main/devices/style/iks30a_stylesheet.css");
    const int kDiscoveryTimeoutMs = 5000;
    const QString kDeviceName("IKS-30A 0034");

    const QMap<float, QString> kErrors = {
        { -1, "Отклонение тока уставки. \nВыбран неверный режим измерения или не подключены токовые зонды" },
        { -2, "Сопротивление велико для выбранного тока или не подключены потенциальные зонды" },
        { -3, "Ток установлен Превышение диапазона измерения сопротивления – во время измерения\n"
              "Превышен диапазон измерения сопротивления – по окончанию измерения" },
        { -4, "Отмена установки тока"},
        { -5, "Перегрев"},
        { -6, "Низкий заряд аккумулятора"},
        { -7, "Низкий заряд ячейки"},
        { -8, "Низкий заряд для выбранного тока"},
    };
}

Device_IKS30A::Device_IKS30A(const QString &title, Device *parent)
    : Device(title, parent)
    , ui(new Ui::Device_IKS30A)
#ifdef Q_OS_WIN
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent)
#endif
{
    ui->setupUi(this);
    setStyleSheet(utils::loadStyleSheet(kStylesheetPath));
    configUi();

#ifdef Q_OS_WIN
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(kDiscoveryTimeoutMs);

    QObject::connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                     [this] (const QBluetoothDeviceInfo &info) {
                        if (info.name() == kDeviceName) {
                            const auto address = info.address().toString();
                            m_devices.insert(address, info);
                            ui->cbDevices->addItem(info.name(), address);
                        }
    });

    QObject::connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
                     [] (QBluetoothDeviceDiscoveryAgent::Error error) {
        qDebug() << "Bluetooth discovery error:" << error;
    });

    QObject::connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, [this] () {
        if (m_spinner) {
            m_spinner->setVisible(false);
        }

        ui->pbConnect->setEnabled(ui->cbDevices->count() > 0);
    });

    QObject::connect(ui->pbUpdate, &QPushButton::clicked, [this] {
        if (m_discoveryAgent->isActive()) {
            m_discoveryAgent->stop();
        }
        ui->cbDevices->clear();
        m_devices.clear();
        m_spinner->setVisible(true);
        m_discoveryAgent->start();
    });

    QObject::connect(ui->pbConnect, &QPushButton::clicked, [this] {
        if (m_controller == nullptr) {
            const auto deviceInfo = m_devices.value(ui->cbDevices->currentData().toString());
            if (deviceInfo.isValid()) {
                m_controller = new BluetoothIKSDevice(deviceInfo, this);
                QObject::connect(m_controller, &BluetoothIKSDevice::connected, [this] {
                    ui->lblConnectionStatus->setText("Соединение установлено");
                    ui->wMeasure->setEnabled(true);
                });
                QObject::connect(m_controller, &BluetoothIKSDevice::disconnected, [this] {
                    ui->wMeasure->setEnabled(false);
                });
                QObject::connect(m_controller, &BluetoothIKSDevice::responseReceived, [this] (const QVariant &response) {
                    ui->stackedWidget_2->setCurrentIndex(1);
                    float result = response.toFloat();
                    if (result < 0) {
                        ui->lblMeasureResult->setText(kErrors.value(result, "Неизвестная ошибка"));
                    }
                    else {
                        ui->lblMeasureResult->setText(QString::number(result * 1000.0f) + " мОм");
                    }

                });
                ui->lblConnectionStatus->setVisible(true);
                ui->lblConnectionStatus->setText("Подключение...");
                m_controller->connect();
            }
        }
    });

    QObject::connect(ui->pbMeasure, &QPushButton::clicked, [this] {
        if (m_controller) {
            const auto measureType = ui->cbMeasureType->currentIndex() ? BluetoothIKSDevice::InductiveResistance
                                                                       : BluetoothIKSDevice::ActiveResistance;
            const QVariant param = ui->cbAmperage->currentData();
            m_controller->sendRequest(measureType, param);
            ui->stackedWidget_2->setCurrentIndex(0);
            ui->lblMeasureStatus->setText("Выполняется измерение...");
        }
    });
#endif

//    QObject::connect(ui->cbDevices, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this] {
//        ui->pbConnect->setEnabled(ui->cbDevices->count() > 0);
//    });
}

Device_IKS30A::~Device_IKS30A()
{
    delete ui;
}

void Device_IKS30A::configUi()
{
    ui->lblTitle->setText(title());

    ui->cbAmperage->addItem("30А", 30000.0f);
    ui->cbAmperage->addItem("10А", 10000.0f);
    ui->cbAmperage->addItem("3А", 3000.0f);
    ui->cbAmperage->addItem("1А", 1000.0f);
    ui->cbAmperage->addItem("200мА", 200.0f);
    ui->cbAmperage->addItem("40мА", 40.0f);
    ui->cbAmperage->addItem("4мА", 4.0f);
    ui->cbAmperage->addItem("400мкА", 0.4f);
    ui->cbAmperage->addItem("80мкА", 0.08f);

    ui->cbMeasureType->addItem("активное");
    ui->cbMeasureType->addItem("индуктивное");

    m_spinner = new QSvgWidget(":/ui/style/spinner.svg");
    m_spinner->setVisible(false);
    m_spinner->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->horizontalLayout_5->addWidget(m_spinner);

//    m_spinner2 = new QSvgWidget(":/ui/style/spinner2.svg", this);
//    m_spinner2->setVisible(true);
//    m_spinner2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    ui->verticalLayout_3->addWidget(m_spinner2);

    ui->pbConnect->setEnabled(false);

    ui->wMeasure->setEnabled(false);
    ui->lblConnectionStatus->setVisible(false);
}
