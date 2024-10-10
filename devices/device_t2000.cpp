#include "device_t2000.h"
#include "ui_device_t2000.h"

#include "utils.h"

#include <QMessageBox>
#include <QDateTime>
#include <QShortcut>

namespace {
    const QString kStylesheetPath(":/main/devices/style/t2000_stylesheet.css");
}

Device_T2000::Device_T2000(const QString &title, const QPixmap &schema, QWidget *parent)
    : Device(title, schema, parent)
    , ui(new Ui::Device_T2000)
    , m_protocolModel(new QStandardItemModel(parent))
{
    ui->setupUi(this);
    setStyleSheet(utils::loadStyleSheet(kStylesheetPath));
    configUi();
}

Device_T2000::~Device_T2000()
{
    delete ui;
}

void Device_T2000::configUi()
{
    ui->lblTitle->setText(title());

    m_protocolModel->setHorizontalHeaderLabels(QStringList() << "Дата"
                                                             << "Объект"
                                                             << "Зав. №"
                                                             << "Сх, пФ"
                                                             << "tg, %"
                                                             << "Utst, В"
                                                             << "Схема"
                                                             << "t, С"
                                                             << "Оператор"
                                                             << "Время");

    QAbstractButton *tvCornerButton = ui->tvProtocol->findChild<QAbstractButton*>();
    if(tvCornerButton){
        auto *layoutCornerButton = new QVBoxLayout(tvCornerButton);
        layoutCornerButton->setContentsMargins(0, 0, 0, 0);
        auto *lblCornerButton = new QLabel("№");
        lblCornerButton->setAlignment(Qt::AlignCenter);
        lblCornerButton->setContentsMargins(0, 0, 0, 0);
        layoutCornerButton->addWidget(lblCornerButton);
    }

    ui->tvProtocol->setModel(m_protocolModel);
    ui->tvProtocol->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tvProtocol->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->tvProtocol->horizontalHeader()->setStretchLastSection(true);
    ui->tvProtocol->verticalHeader()->setFixedWidth(40);
    ui->tvProtocol->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tvProtocol->verticalHeader()->setVisible(true);

    ui->cbSchema->addItem("Прямая");
    ui->cbSchema->addItem("Инверсная");

    ui->cbZone->addItem("Зона 0");
    ui->cbZone->addItem("Зона 1");
    ui->cbZone->addItem("Зона 2");
    ui->cbZone->addItem("Зона 3");

    ui->pbConnect->setEnabled(false);

    //ui->wMeasure->setEnabled(false);
    ui->lblConnectionStatus->setVisible(false);


    auto protocolRecordDeleteHotkey = new QShortcut(Qt::Key_Delete, this);
    QObject::connect(protocolRecordDeleteHotkey, &QShortcut::activated, [this] {
        removeProtocolRecords();
    });

    QObject::connect(ui->pbProtocol, &QPushButton::clicked, [this] {
        insertProtocolRecords();
    });

    QObject::connect(ui->pbMeasure, &QPushButton::clicked, [this] {
        if (ui->sbVoltage->value() < 0) {
            QMessageBox messageBoxSetVoltage(QMessageBox::Warning,
                                             QObject::tr("Тангенс-2000"),
                                             QObject::tr("Укажите значение напряжения испытания!"),
                                             QMessageBox::NoButton,
                                             this);
            messageBoxSetVoltage.exec();
            return;
        }

        QMessageBox messageBoxStart(QMessageBox::Warning,
                                    QObject::tr("Тангенс-2000"),
                                    QObject::tr("Начать проведение испытания? "
                                                "На выводы установки будет подано высокое напряжение."),
                                    QMessageBox::Yes | QMessageBox::No,
                                    this);
        messageBoxStart.setButtonText(QMessageBox::Yes, QObject::tr("Подать напряжение!"));
        messageBoxStart.setButtonText(QMessageBox::No, QObject::tr("Нет"));
        messageBoxStart.exec();
    });
}

void Device_T2000::insertProtocolRecords()
{
    const auto currentDateTime = QDateTime::currentDateTime();
    const auto date = new QStandardItem(currentDateTime.toString("dd.MM.yyyy"));
    date->setTextAlignment(Qt::AlignCenter);
    date->setEditable(false);

    const auto object = new QStandardItem;
    object->setTextAlignment(Qt::AlignCenter);

    const auto serial = new QStandardItem;
    serial->setTextAlignment(Qt::AlignCenter);

    const auto cap = new QStandardItem;
    cap->setTextAlignment(Qt::AlignCenter);
    cap->setEditable(false);

    const auto tg = new QStandardItem;
    tg->setTextAlignment(Qt::AlignCenter);
    tg->setEditable(false);

    const auto voltage = new QStandardItem(ui->sbVoltage->text());
    voltage->setTextAlignment(Qt::AlignCenter);
    voltage->setEditable(false);

    const auto schema = new QStandardItem(ui->cbSchema->currentText());
    schema->setTextAlignment(Qt::AlignCenter);
    schema->setEditable(false);

    const auto temp = new QStandardItem(QString::number(ui->sbTemperature->value()));
    temp->setTextAlignment(Qt::AlignCenter);
    temp->setEditable(false);

    const auto op = new QStandardItem;
    op->setTextAlignment(Qt::AlignCenter);

    const auto time = new QStandardItem(currentDateTime.toString("hh:mm"));
    time->setTextAlignment(Qt::AlignCenter);
    time->setEditable(false);

    m_protocolModel->appendRow(QList<QStandardItem*>() << date
                                                       << object
                                                       << serial
                                                       << cap
                                                       << tg
                                                       << voltage
                                                       << schema
                                                       << temp
                                                       << op
                                                       << time);
}

void Device_T2000::removeProtocolRecords()
{
    const QModelIndexList selection = ui->tvProtocol->selectionModel()->selectedRows();
    for (const auto &selectedIndex : selection) {
        if (selectedIndex.isValid()) {
            m_protocolModel->removeRow(selectedIndex.row());
        }
    }
}












