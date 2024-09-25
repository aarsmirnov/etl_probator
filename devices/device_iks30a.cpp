#include "device_iks30a.h"
#include "ui_device_iks30a.h"

#include "utils.h"

#include "widgets/imageviewer.h"

#include <QDebug>
#include <QDateTime>
#include <QShortcut>

namespace {
    const QString kStylesheetPath(":/main/devices/style/iks30a_stylesheet.css");
    const int kDiscoveryTimeoutMs = 5000;
    const QString kDeviceName("IKS-30A 0034");

    constexpr int kConnectionTimeoutMs = 10 * 1000;

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

Device_IKS30A::Device_IKS30A(const QString &title, const QPixmap &schema, Device *parent)
    : Device(title, schema, parent)
    , ui(new Ui::Device_IKS30A)
    , m_connectionTimer(new QTimer(this))
    , m_VnProtocolModel(new QStandardItemModel(parent))
    , m_SnProtocolModel(new QStandardItemModel(parent))
    , m_NnProtocolModel(new QStandardItemModel(parent))
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
        ui->lblConnectionStatus->setVisible(true);
        ui->lblConnectionStatus->setText("Подключение...");
        m_connectionTimer->start();
        if (m_controller == nullptr) {
            const auto deviceInfo = m_devices.value(ui->cbDevices->currentData().toString());
            if (deviceInfo.isValid()) {
                m_controller = new BluetoothIKSDevice(deviceInfo, this);
                QObject::connect(m_controller, &BluetoothIKSDevice::connected, [this] {
                    ui->lblConnectionStatus->setText("Соединение установлено");
                    ui->pbMeasure->setEnabled(true);
                });
                QObject::connect(m_controller, &BluetoothIKSDevice::disconnected, [this] {
                    ui->pbMeasure->setEnabled(false);
                });
                QObject::connect(m_controller, &BluetoothIKSDevice::responseReceived, [this] (const QVariant &response) {
                 ui->stackedWidget->setCurrentIndex(1);
                    float result = response.toFloat();
                    setMeasureResult(result);
                });
                m_controller->connect();
            }
        }
    });

    QObject::connect(ui->pbMeasure, &QPushButton::clicked, [this] {
        if (m_controller) {
            const auto measureType = ui->chbMeasureType->isChecked() ? BluetoothIKSDevice::InductiveResistance
                                                                     : BluetoothIKSDevice::ActiveResistance;
            const QVariant param = ui->cbAmperage->currentData();
            m_controller->sendRequest(measureType, param);
            //ui->stackedWidget->setCurrentIndex(0);
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

QVector<QStringList> Device_IKS30A::protocol()
{
    const auto model = getCurrentProtocolModel();
    if (model == nullptr) {
        return QVector<QStringList>();
    }

    QVector<QStringList> result;
    for (int i = 0; i < model->rowCount(); i++) {
        QStringList rowItem;
        for (int j = 0; j < model->columnCount(); j++) {
            rowItem << model->item(i, j)->text();
        }
        result.append(rowItem);
    }
    return result;
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

    ui->cbMaterial->addItem("Медь", 235.0f);
    ui->cbMaterial->addItem("Алюминий", 245.0f);

    ui->cbType->addItem("Периодические");

    m_spinner = new QSvgWidget(":/ui/style/spinner.svg");
    m_spinner->setVisible(false);
    m_spinner->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->horizontalLayout_5->addWidget(m_spinner);

    m_VnProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                               << "Объект"
                                                               << "Вид испытаний"
                                                               << "Зав. №"
                                                               << "A0"
                                                               << "B0"
                                                               << "C0"
                                                               << QString { "A0 (t=%1)" }.arg(ui->sbTemp->value())
                                                               << QString { "B0 (t=%1)" }.arg(ui->sbTemp->value())
                                                               << QString { "C0 (t=%1)" }.arg(ui->sbTemp->value())
                                                               << "Оператор"
                                                               << "Время");
    ui->tvVnProtocol->setModel(m_VnProtocolModel);
    ui->tvVnProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvVnProtocol->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvVnProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvVnProtocol->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tvVnProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvVnProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvVnProtocol->verticalHeader()->setVisible(true);

    m_SnProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                 << "Объект"
                                                 << "Вид испытаний"
                                                 << "Зав. №"
                                                 << "A0"
                                                 << "B0"
                                                 << "C0"
                                                 << QString { "A0 (t=%1)" }.arg(ui->sbTemp->value())
                                                 << QString { "B0 (t=%1)" }.arg(ui->sbTemp->value())
                                                 << QString { "C0 (t=%1)" }.arg(ui->sbTemp->value())
                                                 << "Оператор"
                                                 << "Время");
    ui->tvSnProtocol->setModel(m_SnProtocolModel);
    ui->tvSnProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvSnProtocol->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvSnProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvSnProtocol->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tvSnProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvSnProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvSnProtocol->verticalHeader()->setVisible(true);

    m_NnProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                 << "Объект"
                                                 << "Вид испытаний"
                                                 << "Зав. №"
                                                 << "A0"
                                                 << "B0"
                                                 << "C0"
                                                 << QString { "A0 (t=%1)" }.arg(ui->sbTemp->value())
                                                 << QString { "B0 (t=%1)" }.arg(ui->sbTemp->value())
                                                 << QString { "C0 (t=%1)" }.arg(ui->sbTemp->value())
                                                 << "Оператор"
                                                 << "Время");
    ui->tvNnProtocol->setModel(m_NnProtocolModel);
    ui->tvNnProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvNnProtocol->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvNnProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvNnProtocol->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tvNnProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvNnProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvNnProtocol->verticalHeader()->setVisible(true);


//    m_spinner2 = new QSvgWidget(":/ui/style/spinner2.svg", this);
//    m_spinner2->setVisible(true);
//    m_spinner2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    ui->verticalLayout_3->addWidget(m_spinner2);

    ui->pbConnect->setEnabled(false);

    //ui->wMeasure->setEnabled(false);
    ui->pbMeasure->setEnabled(false);
    ui->lblConnectionStatus->setVisible(false);

    setMeasureResult();

    QObject::connect(ui->tbHelp, &QToolButton::clicked, [this] {
        ImageViewer viewer;
        viewer.setWindowTitle(QObject::tr("Просмотр схемы"));
        viewer.setImage(schema());
        viewer.exec();
    });

    QObject::connect(ui->sbTemp, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int value) {
        const auto model = getCurrentProtocolModel();
        if (model == nullptr) {
            return;
        }

        model->horizontalHeaderItem(7)->setText(QString { "A0 (t=%1)" }.arg(value));
        model->horizontalHeaderItem(8)->setText(QString { "B0 (t=%1)" }.arg(value));
        model->horizontalHeaderItem(9)->setText(QString { "C0 (t=%1)" }.arg(value));

        for (int i = 0; i < model->rowCount(); i++) {
            model->item(i, 7)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
            model->item(i, 8)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
            model->item(i, 9)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
        }
    });

    QObject::connect(ui->cbMaterial, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this] {
        const auto model = getCurrentProtocolModel();
        if (model == nullptr) {
            return;
        }

        for (int i = 0; i < model->rowCount(); i++) {
            model->item(i, 7)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
            model->item(i, 8)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
            model->item(i, 9)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
        }
    });

    QObject::connect(ui->pbProtocol, &QPushButton::clicked, [this] {
        pushEvent("Добавлена новая запись в таблицу протокола");
        insertProtocolRecords();
    });

    auto protocolRecordDeleteHotkey = new QShortcut(Qt::Key_Delete, this);
    QObject::connect(protocolRecordDeleteHotkey, &QShortcut::activated, [this] {
        pushEvent("Удалена запись из таблицы протокола");
        removeProtocolRecords();
    });

    QObject::connect(m_VnProtocolModel, &QStandardItemModel::itemChanged, [this] (QStandardItem *item) {
        if (item->column() == 4 || item->column() == 5 || item->column() == 6) {
            m_VnProtocolModel->item(item->row(), item->column() + 3)->setText(QString::number(getResistanceForTemp(m_lastMeasureResult)));
        }
    });

    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(kConnectionTimeoutMs);
    QObject::connect(m_connectionTimer, &QTimer::timeout, [this] {
        ui->lblConnectionStatus->setText("Не удалось подключиться к устройству");
        m_controller->disconnect();
    });
}

void Device_IKS30A::setMeasureResult(float val)
{
    if (val >= 0) {
        m_lastMeasureResult = val;
    }
    const QString result = (val < 0) ? "----" : QString::number(val * 100000.0f);
    ui->lblMeasureResult->setText(result + " мкОм");
}

void Device_IKS30A::insertProtocolRecords()
{
    const auto model = getCurrentProtocolModel();
    if (model == nullptr) {
        return;
    }

    const auto currentDateTime = QDateTime::currentDateTime();
    const auto date = new QStandardItem(currentDateTime.toString("dd.MM.yyyy"));
    date->setTextAlignment(Qt::AlignCenter);
    date->setEditable(false);

    const auto object = new QStandardItem(ui->leObject->text());
    object->setTextAlignment(Qt::AlignCenter);
    object->setEditable(false);

    const auto type = new QStandardItem(ui->cbType->currentText());
    type->setTextAlignment(Qt::AlignCenter);
    type->setEditable(false);

    const auto serial = new QStandardItem(ui->leSerialNumber->text());
    serial->setTextAlignment(Qt::AlignCenter);
    serial->setEditable(false);

    const auto a0 = new QStandardItem;
    a0->setTextAlignment(Qt::AlignCenter);

    const auto b0 = new QStandardItem;
    b0->setTextAlignment(Qt::AlignCenter);

    const auto c0 = new QStandardItem;
    c0->setTextAlignment(Qt::AlignCenter);

    const auto a0_t = new QStandardItem(QString::number(getResistanceForTemp(m_lastMeasureResult)));
    a0_t->setTextAlignment(Qt::AlignCenter);
    a0_t->setEditable(false);

    const auto b0_t = new QStandardItem(QString::number(getResistanceForTemp(m_lastMeasureResult)));
    b0_t->setTextAlignment(Qt::AlignCenter);
    b0_t->setEditable(false);

    const auto c0_t = new QStandardItem(QString::number(getResistanceForTemp(m_lastMeasureResult)));
    c0_t->setTextAlignment(Qt::AlignCenter);
    c0_t->setEditable(false);

    const auto op = new QStandardItem(ui->leOperator->text());
    op->setTextAlignment(Qt::AlignCenter);
    op->setEditable(false);

    const auto time = new QStandardItem(currentDateTime.toString("hh:mm"));
    time->setTextAlignment(Qt::AlignCenter);
    time->setEditable(false);

    model->appendRow(QList<QStandardItem*>() << date
                                             << object
                                             << type
                                             << serial
                                             << a0
                                             << b0
                                             << c0
                                             << a0_t
                                             << b0_t
                                             << c0_t
                                             << op
                                             << time);
}

void Device_IKS30A::removeProtocolRecords()
{
    const QModelIndexList selection = ui->tvVnProtocol->selectionModel()->selectedRows();
    for (const auto &selectedIndex : selection) {
        if (selectedIndex.isValid()) {
            m_VnProtocolModel->removeRow(selectedIndex.row());
        }
    }
}

float Device_IKS30A::getResistanceForTemp(float baseResistance)
{
    const float k = ui->cbMaterial->currentData().toFloat();
    const float result = (baseResistance * (k + 20.0f)) / (k + static_cast<float>(ui->sbTemp->value()));
    return result;
}

QStandardItemModel *Device_IKS30A::getCurrentProtocolModel()
{
    switch (ui->twProtocol->currentIndex()) {
    case 0:
        return m_VnProtocolModel;
    case 1:
        return m_SnProtocolModel;
    case 2:
        return m_NnProtocolModel;
    default:
        return nullptr;
    }
}
