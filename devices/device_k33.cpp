#include "device_k33.h"
#include "ui_device_k33.h"

#include "utils.h"

#include "k33serialpackets.h"

#include <QSerialPortInfo>
#include <QDateTime>
#include <QDebug>

namespace {
    const QString kStylesheetPath(":/main/devices/style/k33_stylesheet.css");
}

Device_K33::Device_K33(const QString &title, const QPixmap &schema, Device *parent)
    : Device(title, schema, parent)
    , ui(new Ui::Device_K33)
    , m_terminalModel(new QStandardItemModel(parent))
    , m_ktProtocolModel(new QStandardItemModel(parent))
    , m_hhProtocolModel(new QStandardItemModel(parent))
    , m_kzProtocolModel(new QStandardItemModel(parent))

{
    ui->setupUi(this);
    setStyleSheet(utils::loadStyleSheet(kStylesheetPath));
    configUi();
}

Device_K33::~Device_K33()
{
    delete ui;
}

QVector<QStringList> Device_K33::protocol()
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

void Device_K33::configUi()
{
    ui->lblTitle->setText(title());

    ui->pbConnect->setEnabled(false);
    ui->wControl->setEnabled(false);
    ui->lblConnectionStatus->setVisible(false);

    ui->tbElectricWarning->setEnabled(false);

    ui->cbMode->addItem("КТ", 0x00);
    ui->cbMode->addItem("ХХ", 0x01);
    ui->cbMode->addItem("КЗ", 0x02);

    ui->cbVoltage->addItem("Фазное", 0x00);
    ui->cbVoltage->addItem("Линейное", 0x01);

    ui->cbUnnA->addItem("5В", 0x00);
    ui->cbUnnA->addItem("50В", 0x01);
    ui->cbUnnA->addItem("500В", 0x02);

    ui->cbUnnB->addItem("5В", 0x00);
    ui->cbUnnB->addItem("50В", 0x01);
    ui->cbUnnB->addItem("500В", 0x02);

    ui->cbUnnC->addItem("5В", 0x00);
    ui->cbUnnC->addItem("50В", 0x01);
    ui->cbUnnC->addItem("500В", 0x02);

    ui->cbIa->addItem("20мА", 0x00);
    ui->cbIa->addItem("200мА", 0x01);
    ui->cbIa->addItem("2А", 0x02);
    ui->cbIa->addItem("20А", 0x03);

    ui->cbIb->addItem("20мА", 0x00);
    ui->cbIb->addItem("200мА", 0x01);
    ui->cbIb->addItem("2А", 0x02);
    ui->cbIb->addItem("20А", 0x03);

    ui->cbIc->addItem("20мА", 0x00);
    ui->cbIc->addItem("200мА", 0x01);
    ui->cbIc->addItem("2А", 0x02);
    ui->cbIc->addItem("20А", 0x03);

    setParamMode(true);

    m_terminalModel->setColumnCount(3);

    ui->tvTerminal->setModel(m_terminalModel);
    ui->tvTerminal->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvTerminal->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvTerminal->horizontalHeader()->setStretchLastSection(true);
    ui->tvTerminal->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tvTerminal->horizontalHeader()->setVisible(false);
    ui->tvTerminal->verticalHeader()->setVisible(false);

    m_ktProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                 << "Объект"
                                                 << "Вид испытаний"
                                                 << "Зав. №"
                                                 << "U В А(АВ), В"
                                                 << "U В В(ВС), В"
                                                 << "U В С(СА), В"
                                                 << "U Н А(АВ), В"
                                                 << "U Н В(ВС), В"
                                                 << "U С С(СА), В"
                                                 << "Кт А"
                                                 << "Кт В"
                                                 << "Кт С"
                                                 << "Оператор"
                                                 << "Время");
    ui->tvKtProtocol->setModel(m_ktProtocolModel);
    ui->tvKtProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvKtProtocol->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvKtProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvKtProtocol->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tvKtProtocol->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
    ui->tvKtProtocol->horizontalHeader()->setSectionResizeMode(11, QHeaderView::ResizeToContents);
    ui->tvKtProtocol->horizontalHeader()->setSectionResizeMode(12, QHeaderView::ResizeToContents);
    ui->tvKtProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvKtProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvKtProtocol->verticalHeader()->setVisible(true);

    m_hhProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                               << "Объект"
                                                               << "Вид испытаний"
                                                               << "Зав. №"
                                                               << "U AB , В"
                                                               << "U BC , В"
                                                               << "U CA , В"
                                                               << "I A , А"
                                                               << "I B , А"
                                                               << "I C , А"
                                                               << "P A , Вт"
                                                               << "P B , Вт"
                                                               << "P C , Вт"
                                                               << "Оператор"
                                                               << "Время");
    ui->tvHhProtocol->setModel(m_hhProtocolModel);
    ui->tvHhProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvHhProtocol->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvHhProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvHhProtocol->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tvHhProtocol->horizontalHeader()->setSectionResizeMode(13, QHeaderView::ResizeToContents);
    ui->tvHhProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvHhProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvHhProtocol->verticalHeader()->setVisible(true);

    m_kzProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                               << "Объект"
                                                               << "Вид испытаний"
                                                               << "Зав. №"
                                                               << "U A , В"
                                                               << "U B , В"
                                                               << "U C , В"
                                                               << "I A , А"
                                                               << "I B , А"
                                                               << "I C , А"
                                                               << "Z A , Вт"
                                                               << "Z B , Вт"
                                                               << "Z C , Вт"
                                                               << "Оператор"
                                                               << "Время");
    ui->tvKzProtocol->setModel(m_kzProtocolModel);
    ui->tvKzProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvKzProtocol->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvKzProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvKzProtocol->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tvKzProtocol->horizontalHeader()->setSectionResizeMode(13, QHeaderView::ResizeToContents);
    ui->tvKzProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvKzProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvKzProtocol->verticalHeader()->setVisible(true);

    QObject::connect(ui->cbMode, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this] (int index) {
        setParamMode(index == 0);
    });

    QObject::connect(ui->pbUpdate, &QPushButton::clicked, [this] {
        ui->cbDevices->clear();
        pushEvent("Поиск доступных COM портов");
        const auto ports = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &port : ports) {
            ui->cbDevices->addItem(port.portName());
            pushEvent(QString{ "%1 : %2"}.arg(port.portName()).arg(port.description()));
        }
    });

    QObject::connect(ui->cbDevices, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this] {
        ui->pbConnect->setEnabled(ui->cbDevices->count() > 0);
    });

    QObject::connect(ui->pbConnect, &QPushButton::clicked, [this] {
        if (m_serialRequester == nullptr || !m_serialRequester->isOpen()) {
            const QString currentPort = ui->cbDevices->currentText();
            m_serialRequester = new K33SerialRequester(ui->cbDevices->currentText(), this);
            pushEvent(QString{ "Открытие COM порта %1"}.arg(currentPort));
            if (m_serialRequester->open()) {
                pushEvent(QString{ "COM порт %1 успешно открыт"}.arg(currentPort));
                ui->wControl->setEnabled(true);
                QObject::connect(m_serialRequester, &K33SerialRequester::dataReceived, [this] (const QByteArray &data){
                    qDebug() << "New packet: " << data.toHex();
                    if (m_readyToMeasure == false) {
                        const auto packet = K33Packets::makeStartMeasurePacket();
                        m_serialRequester->writeRequest(packet);
                        m_readyToMeasure = true;
                    } else {
                        m_measureResult[ui->cbMode->currentData().toInt()] = K33Packets::parseMeasureResultPacket(data, ui->cbMode->currentData().toInt());//parseMeasureKtResultPacket(data);
                        updateMeasureResult();
                        pushEvent("Измерение завершено");
                    }
                });
            }
            else {
                pushEvent(QString{ "COM порт %1 не удалось открыть"}.arg(currentPort));
            }
        }
    });

    QObject::connect(ui->pbMeasure, &QPushButton::clicked, [this] {
        m_readyToMeasure = false;
        K33Packets::MeasureParams params{};
        params.mode_code = ui->cbMode->currentData().toInt();
        params.voltage_code = ui->cbVoltage->currentData().toInt();
        if (params.mode_code == 0x00) {
            params.u_a = ui->cbUnnA->currentData().toInt();
            params.u_b = ui->cbUnnB->currentData().toInt();
            params.u_c = ui->cbUnnC->currentData().toInt();
        }
        else {
            params.i_a = ui->cbIa->currentData().toInt();
            params.i_b = ui->cbIb->currentData().toInt();
            params.i_c = ui->cbIc->currentData().toInt();
        }

        const auto packet = K33Packets::makeSetMeasureParamsPacket(params);
        m_serialRequester->writeRequest(packet);
        pushEvent("Выполняется измерение");
    });

    QObject::connect(ui->pbProtocol, &QPushButton::clicked, [this] {
        pushEvent("Добавлена новая запись в таблицу протокола");
        insertProtocolRecords();
    });
}

void Device_K33::setParamMode(bool enable)
{
    ui->cbUnnA->setEnabled(enable);
    ui->cbUnnB->setEnabled(enable);
    ui->cbUnnC->setEnabled(enable);

    ui->cbIa->setEnabled(!enable);
    ui->cbIb->setEnabled(!enable);
    ui->cbIc->setEnabled(!enable);
}

void Device_K33::updateMeasureResult()
{
    m_terminalModel->clear();

    switch (ui->cbMode->currentData().toInt()) {
    case 0x00:
        updateKtMeasureResult();
        break;
    case 0x01:
        updateHhMeasureResult();
        break;
    case 0x02:
        updateKzMeasureResult();
        break;
    default:
        break;
    }
}

void Device_K33::updateKtMeasureResult()
{
    const auto result = m_measureResult.value(0);

    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("fba"))
                                                        << new QStandardItem(result.value("fbb"))
                                                        << new QStandardItem(result.value("fbc")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("fna"))
                                                        << new QStandardItem(result.value("fnb"))
                                                        << new QStandardItem(result.value("fnc")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("ubab"))
                                                        << new QStandardItem(result.value("ubbc"))
                                                        << new QStandardItem(result.value("ubca")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("unab"))
                                                        << new QStandardItem(result.value("unbc"))
                                                        << new QStandardItem(result.value("unca")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("kkt")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("ktab"))
                                                        << new QStandardItem(result.value("ktbc"))
                                                        << new QStandardItem(result.value("ktca")));
}

void Device_K33::updateHhMeasureResult()
{
    const auto result = m_measureResult.value(1);

    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("fua"))
                                                        << new QStandardItem(result.value("fub"))
                                                        << new QStandardItem(result.value("fuc")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("fia"))
                                                        << new QStandardItem(result.value("fib"))
                                                        << new QStandardItem(result.value("fic")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("uab"))
                                                        << new QStandardItem(result.value("ubc"))
                                                        << new QStandardItem(result.value("uca")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("ia"))
                                                        << new QStandardItem(result.value("ib"))
                                                        << new QStandardItem(result.value("ic")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("pa"))
                                                        << new QStandardItem(result.value("pb"))
                                                        << new QStandardItem(result.value("pc")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("xxa"))
                                                        << new QStandardItem(result.value("xxb"))
                                                        << new QStandardItem(result.value("xxc")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("xa"))
                                                        << new QStandardItem(result.value("xb"))
                                                        << new QStandardItem(result.value("xc")));
}

void Device_K33::updateKzMeasureResult()
{
    const auto result = m_measureResult.value(2);

    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("fua"))
                                                        << new QStandardItem(result.value("fub"))
                                                        << new QStandardItem(result.value("fuc")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("fia"))
                                                        << new QStandardItem(result.value("fib"))
                                                        << new QStandardItem(result.value("fic")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("uab"))
                                                        << new QStandardItem(result.value("ubc"))
                                                        << new QStandardItem(result.value("uca")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("ia"))
                                                        << new QStandardItem(result.value("ib"))
                                                        << new QStandardItem(result.value("ic")));
    m_terminalModel->appendRow(QList<QStandardItem*>() << new QStandardItem(result.value("zab"))
                                                        << new QStandardItem(result.value("zbc"))
                                                        << new QStandardItem(result.value("zca")));
}

void Device_K33::insertProtocolRecords()
{
    switch (ui->cbMode->currentData().toInt()) {
    case 0x00:
        insertKtProtocolRecords();
        break;
    case 0x01:
        insertHhProtocolRecords();
        break;
    case 0x02:
        insertKzProtocolRecords();
        break;
    default:
        break;
    }
}

void Device_K33::insertKtProtocolRecords()
{
    const auto result = m_measureResult.value(0);
    if (result.size() == 0) {
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

    const auto ubab = new QStandardItem(result.value("ubab").split(" ").value(1));
    ubab->setTextAlignment(Qt::AlignCenter);
    ubab->setEditable(false);

    const auto ubbc = new QStandardItem(result.value("ubbc").split(" ").value(1));
    ubbc->setTextAlignment(Qt::AlignCenter);
    ubbc->setEditable(false);

    const auto ubca = new QStandardItem(result.value("ubca").split(" ").value(1));
    ubca->setTextAlignment(Qt::AlignCenter);
    ubca->setEditable(false);

    const auto unab = new QStandardItem(result.value("unab").split(" ").value(1));
    unab->setTextAlignment(Qt::AlignCenter);
    unab->setEditable(false);

    const auto unbc = new QStandardItem(result.value("unbc").split(" ").value(1));
    unbc->setTextAlignment(Qt::AlignCenter);
    unbc->setEditable(false);

    const auto unca = new QStandardItem(result.value("unca").split(" ").value(1));
    unca->setTextAlignment(Qt::AlignCenter);
    unca->setEditable(false);

    const auto ktab = new QStandardItem(result.value("ktab").split(" ").value(1));
    ktab->setTextAlignment(Qt::AlignCenter);
    ktab->setEditable(false);

    const auto ktbc = new QStandardItem(result.value("ktbc").split(" ").value(1));
    ktbc->setTextAlignment(Qt::AlignCenter);
    ktbc->setEditable(false);

    const auto ktca = new QStandardItem(result.value("ktca").split(" ").value(1));
    ktca->setTextAlignment(Qt::AlignCenter);
    ktca->setEditable(false);

    const auto op = new QStandardItem(ui->leOperator->text());
    op->setTextAlignment(Qt::AlignCenter);
    op->setEditable(false);

    const auto time = new QStandardItem(currentDateTime.toString("hh:mm"));
    time->setTextAlignment(Qt::AlignCenter);
    time->setEditable(false);

    m_ktProtocolModel->appendRow(QList<QStandardItem*>() << date
                                                         << object
                                                         << type
                                                         << serial
                                                         << ubab
                                                         << ubbc
                                                         << ubca
                                                         << unab
                                                         << unbc
                                                         << unca
                                                         << ktab
                                                         << ktbc
                                                         << ktca
                                                         << op
                                                         << time);
}

void Device_K33::insertHhProtocolRecords()
{
    const auto result = m_measureResult.value(1);
    if (result.size() == 0) {
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

    const auto uab = new QStandardItem(result.value("uab").split(" ").value(1));
    uab->setTextAlignment(Qt::AlignCenter);
    uab->setEditable(false);

    const auto ubc = new QStandardItem(result.value("ubc").split(" ").value(1));
    ubc->setTextAlignment(Qt::AlignCenter);
    ubc->setEditable(false);

    const auto uca = new QStandardItem(result.value("uca").split(" ").value(1));
    uca->setTextAlignment(Qt::AlignCenter);
    uca->setEditable(false);

    const auto ia = new QStandardItem(result.value("ia").split(" ").value(1));
    ia->setTextAlignment(Qt::AlignCenter);
    ia->setEditable(false);

    const auto ib = new QStandardItem(result.value("ib").split(" ").value(1));
    ib->setTextAlignment(Qt::AlignCenter);
    ib->setEditable(false);

    const auto ic = new QStandardItem(result.value("ic").split(" ").value(1));
    ic->setTextAlignment(Qt::AlignCenter);
    ic->setEditable(false);

    const auto pa = new QStandardItem(result.value("pa").split(" ").value(1));
    pa->setTextAlignment(Qt::AlignCenter);
    pa->setEditable(false);

    const auto pb = new QStandardItem(result.value("pb").split(" ").value(1));
    pb->setTextAlignment(Qt::AlignCenter);
    pb->setEditable(false);

    const auto pc = new QStandardItem(result.value("pc").split(" ").value(1));
    pc->setTextAlignment(Qt::AlignCenter);
    pc->setEditable(false);

    const auto op = new QStandardItem(ui->leOperator->text());
    op->setTextAlignment(Qt::AlignCenter);
    op->setEditable(false);

    const auto time = new QStandardItem(currentDateTime.toString("hh:mm"));
    time->setTextAlignment(Qt::AlignCenter);
    time->setEditable(false);

    m_hhProtocolModel->appendRow(QList<QStandardItem*>() << date
                                                          << object
                                                          << type
                                                          << serial
                                                          << uab
                                                          << ubc
                                                          << uca
                                                          << ia
                                                          << ib
                                                          << ic
                                                          << pa
                                                          << pb
                                                          << pc
                                                          << op
                                                          << time);
}

void Device_K33::insertKzProtocolRecords()
{
    const auto result = m_measureResult.value(2);
    if (result.size() == 0) {
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

    const auto uab = new QStandardItem(result.value("uab").split(" ").value(1));
    uab->setTextAlignment(Qt::AlignCenter);
    uab->setEditable(false);

    const auto ubc = new QStandardItem(result.value("ubc").split(" ").value(1));
    ubc->setTextAlignment(Qt::AlignCenter);
    ubc->setEditable(false);

    const auto uca = new QStandardItem(result.value("uca").split(" ").value(1));
    uca->setTextAlignment(Qt::AlignCenter);
    uca->setEditable(false);

    const auto ia = new QStandardItem(result.value("ia").split(" ").value(1));
    ia->setTextAlignment(Qt::AlignCenter);
    ia->setEditable(false);

    const auto ib = new QStandardItem(result.value("ib").split(" ").value(1));
    ib->setTextAlignment(Qt::AlignCenter);
    ib->setEditable(false);

    const auto ic = new QStandardItem(result.value("ic").split(" ").value(1));
    ic->setTextAlignment(Qt::AlignCenter);
    ic->setEditable(false);

    const auto zab = new QStandardItem(result.value("zab").split(" ").value(1));
    zab->setTextAlignment(Qt::AlignCenter);
    zab->setEditable(false);

    const auto zbc = new QStandardItem(result.value("zbc").split(" ").value(1));
    zbc->setTextAlignment(Qt::AlignCenter);
    zbc->setEditable(false);

    const auto zca = new QStandardItem(result.value("zca").split(" ").value(1));
    zca->setTextAlignment(Qt::AlignCenter);
    zca->setEditable(false);

    const auto op = new QStandardItem(ui->leOperator->text());
    op->setTextAlignment(Qt::AlignCenter);
    op->setEditable(false);

    const auto time = new QStandardItem(currentDateTime.toString("hh:mm"));
    time->setTextAlignment(Qt::AlignCenter);
    time->setEditable(false);

    m_kzProtocolModel->appendRow(QList<QStandardItem*>() << date
                                                          << object
                                                          << type
                                                          << serial
                                                          << uab
                                                          << ubc
                                                          << uca
                                                          << ia
                                                          << ib
                                                          << ic
                                                          << zab
                                                          << zbc
                                                          << zca
                                                          << op
                                                          << time);
}

void Device_K33::removeProtocolRecords()
{
    QTableView *currentView = nullptr;
    switch (ui->twProtocol->currentIndex()) {
    case 0:
        currentView = ui->tvKtProtocol;
    case 1:
        currentView = ui->tvHhProtocol;
    case 2:
        currentView = ui->tvKzProtocol;
    default:
        return;
    }

    const QModelIndexList selection = currentView->selectionModel()->selectedRows();
    for (const auto &selectedIndex : selection) {
        if (selectedIndex.isValid()) {
            currentView->model()->removeRow(selectedIndex.row());
        }
    }
}

QStandardItemModel *Device_K33::getCurrentProtocolModel()
{
    switch (ui->twProtocol->currentIndex()) {
    case 0:
        return m_ktProtocolModel;
    case 1:
        return m_hhProtocolModel;
    case 2:
        return m_kzProtocolModel;
    default:
        return nullptr;
    }
}
