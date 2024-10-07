#include "device_k33.h"
#include "ui_device_k33.h"

#include "utils.h"

namespace {
    const QString kStylesheetPath(":/main/devices/style/k33_stylesheet.css");
}

Device_K33::Device_K33(const QString &title, const QPixmap &schema, Device *parent)
    : Device(title, schema, parent)
    , ui(new Ui::Device_K33)
    , m_terminalModel(new QStandardItemModel(parent))
    , m_ktProtocolModel(new QStandardItemModel(parent))

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
    return Device::protocol();
}

void Device_K33::configUi()
{
    ui->lblTitle->setText(title());

    ui->pbConnect->setEnabled(false);
    ui->wControl->setEnabled(false);
    ui->lblConnectionStatus->setVisible(false);

    ui->tbElectricWarning->setEnabled(false);

    ui->cbMode->addItem("КТ");
    ui->cbMode->addItem("ХХ");
    ui->cbMode->addItem("КЗ");

    ui->cbVoltage->addItem("Линейное");
    ui->cbVoltage->addItem("Фазное");

    ui->cbUnnA->addItem("5В", 5.0f);
    ui->cbUnnA->addItem("50В", 50.0f);
    ui->cbUnnA->addItem("500В", 500.0f);

    ui->cbUnnB->addItem("5В", 5.0f);
    ui->cbUnnB->addItem("50В", 50.0f);
    ui->cbUnnB->addItem("500В", 500.0f);

    ui->cbUnnC->addItem("5В", 5.0f);
    ui->cbUnnC->addItem("50В", 50.0f);
    ui->cbUnnC->addItem("500В", 500.0f);

    ui->cbIa->addItem("20мА", 0.02f);
    ui->cbIa->addItem("200мА", 0.2f);
    ui->cbIa->addItem("2А", 2.0f);
    ui->cbIa->addItem("20А", 20.0f);

    ui->cbIb->addItem("20мА", 0.02f);
    ui->cbIb->addItem("200мА", 0.2f);
    ui->cbIb->addItem("2А", 2.0f);
    ui->cbIb->addItem("20А", 20.0f);

    ui->cbIc->addItem("20мА", 0.02f);
    ui->cbIc->addItem("200мА", 0.2f);
    ui->cbIc->addItem("2А", 2.0f);
    ui->cbIc->addItem("20А", 20.0f);

    setParamMode(true);

//        m_VnProtocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
//                                                               << "Объект"
//                                                               << "Вид испытаний"
//                                                               << "Зав. №"
//                                                               << "A0"
//                                                               << "B0"
//                                                               << "C0"
//                                                               << QString { "A0 (t=%1)" }.arg(ui->sbTemp->value())
//                                                               << QString { "B0 (t=%1)" }.arg(ui->sbTemp->value())
//                                                               << QString { "C0 (t=%1)" }.arg(ui->sbTemp->value())
//                                                               << "Оператор"
//                                                               << "Время");

    m_terminalModel->setColumnCount(3);

    ui->tvTerminal->setModel(m_terminalModel);
    ui->tvTerminal->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvTerminal->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvTerminal->horizontalHeader()->setStretchLastSection(true);
    ui->tvTerminal->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tvTerminal->horizontalHeader()->setVisible(false);

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
    ui->tvKtProtocol->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tvKtProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvKtProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvKtProtocol->verticalHeader()->setVisible(true);

    QObject::connect(ui->cbMode, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this] (int index) {
        setParamMode(index == 0);
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
