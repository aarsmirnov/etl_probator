#include "core.h"

#include <stdexcept>

#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QMetaEnum>
#include <QPair>
#include <QUuid>
#include <QTimer>

#include <QDebug>

#include "logfile.h"
#include "modbusmaster.h"

QMap<Core::Blocks, QString> Core::IndicationErrors = {
    {Core::Blocks::VOLTAGE, QObject::tr("Сетевое напряжение вышло из допустимого диапазона.")},
    {Core::Blocks::CURRENT, QObject::tr("Потребляемый из сети ток превысил максимальное значение.")},
    {Core::Blocks::DOORS, QObject::tr("Не закрыты изолирующие двери.")},
    {Core::Blocks::EMERGENCY_BUTTON, QObject::tr("Нажата аварийная кнопка.")},
    {Core::Blocks::GND, QObject::tr("Не подключено защитное заземление.")},
    {Core::Blocks::WORK_GND, QObject::tr("Не подключено рабочее заземление.")},
};


namespace  {
    const QMap<QString, QString> kTestIcon = {
        { "Измерение параметров трансформаторов Коэффициент 3.3", "sch2" },
        { "Высоковольтные испытания", "sch1" },
        { "Испытание устройств РПН (ПКР-2М)", "sch3" },
        { "Измерение омического сопротивления ИКС-30А", "sch6" },
        { "Измерение диэлектрических потерь (Тангенс 2000)", "sch7" },
        { "Измерение параметров силовых трансформаторов (СЭИТ-4М-К540)", "sch5" },
    };
}

Core::Core(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(QDir::currentPath() + "/probator_config.ini", QSettings::IniFormat))
    , m_modbus_computer(new ModbusMaster(this))
{
    qRegisterMetaType<ProbationScheme>();
    qRegisterMetaType<ProbationType>();
    m_settings->setIniCodec("utf-8");

    Logger.Log("Session started");
    syncSettings();
    loadProbationData();
    m_default_language = generalSetting("default_language").toString();

    m_excel_protocol = generalSetting("excel_file").toString();

    m_modbus_computer->setModbusLabParameters(
                generalSetting("com_port").toInt(),
                generalSetting("baud_rate").toInt(),
                generalSetting("stop_bits").toInt(),
                generalSetting("data_bits").toInt(),
                generalSetting("parity").toString(),
                generalSetting("response_time").toInt(),
                generalSetting("address").toInt());
#ifdef DEV_MODE
    return;
#endif

    ModbusMaster::ModbusError connection_answer = m_modbus_computer->connectModbus();
    if(connection_answer != ModbusMaster::ModbusError::NO_ERROR)
        throw std::runtime_error(ModbusMaster::ModbusErrorText[connection_answer].toUtf8().data());

    QPair<QPair<QString, QString>, ModbusMaster::ModbusError> ID_answer = m_modbus_computer->readID();
    if(ID_answer.second != ModbusMaster::ModbusError::NO_ERROR)
        throw std::runtime_error(ModbusMaster::ModbusErrorText[ID_answer.second].toUtf8().data());

    if(ID_answer.first.first.toInt() != generalSetting("serial_number").toInt())
        throw std::runtime_error(QObject::tr("Серийный номер лаборатории, указанный в программе, не совпадает с установленным на данном устройстве.").toUtf8().data());

    if(ID_answer.first.second.toInt() != generalSetting("controller_version").toInt())
        throw std::runtime_error(QObject::tr("Версия ПО контроллера, указанная в программе, не совпадает с установленной на данном устройстве.").toUtf8().data());

    if (!setupCommandMode())
        throw std::runtime_error(QObject::tr("Не получилось устаовить режим s_MANUAL, перезапустите программу").toUtf8().data());

    m_polling_timer = new QTimer(this);
    m_polling_timer->setInterval(generalSetting("polling_interval_ms").toInt());
    connect(m_polling_timer, &QTimer::timeout, this, &Core::poller);
    //m_polling_timer->start();
}

Core::~Core()
{
    //m_polling_timer->stop();
}

QVariant Core::generalSetting(const QString &key) const
{
    return m_settings->value(QString("general/%1").arg(key));
}

QVariantMap Core::getProbationData() const
{
    return m_probation_data;
}

void Core::setupProbation(const QBitArray &outputs)
{
    m_modbus_outputs = outputs;
}

void Core::poller()
{
    QPair<QBitArray, ModbusMaster::ModbusError> modbus_answer = m_modbus_computer->readRegister(input_register_adress);
    Logger.Log(QString("Modbus answer[blockings]: %1").arg(BitsToString(modbus_answer.first)));
    if (modbus_answer.second == ModbusMaster::ModbusError::NO_ERROR)
        emit switchIndication(modbus_answer.first);
    else
    {
        Logger.Log(QString("Input register read error: %1").arg(ModbusMaster::ModbusErrorText[modbus_answer.second]));
        emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_answer.second]);
    }
}

void Core::start()
{
    Logger.Log("START pressed");
    if (!setupCommandMode()) {
        const QString errText = tr("Unexpected: command mode error, try later");
        Logger.Log(errText);
        emit showErrorDialog(errText);
        return;
    }

    ModbusMaster::ModbusError modbus_error = m_modbus_computer->writeRegister(output_register_adress, m_modbus_outputs);
    if(modbus_error == ModbusMaster::ModbusError::NO_ERROR)
    {
        QPair<QBitArray, ModbusMaster::ModbusError> modbus_answer = m_modbus_computer->readRegister(output_register_adress);
        if(modbus_answer.second == ModbusMaster::ModbusError::NO_ERROR)
        {
            switchVoltageWarning(modbus_answer.first);
            return; //OK
        }
        modbus_error = modbus_answer.second;
    }

    Logger.Log(ModbusMaster::ModbusErrorText[modbus_error]);
    emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]);
}

void Core::stop()
{
    Logger.Log("STOP requested");
    ModbusMaster::ModbusError modbus_error = m_modbus_computer->writeRegister(output_register_adress, QBitArray(16, false));
    if (modbus_error == ModbusMaster::ModbusError::NO_ERROR)
    {
        QPair<QBitArray, ModbusMaster::ModbusError> modbus_answer = m_modbus_computer->readRegister(output_register_adress);
        if (modbus_answer.second == ModbusMaster::ModbusError::NO_ERROR)
        {
            switchVoltageWarning(modbus_answer.first);
            return; // OK
        }
        modbus_error = modbus_answer.second;
    }

    Logger.Log(ModbusMaster::ModbusErrorText[modbus_error]);
    emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_error]);
}

void Core::switchGreenLED(bool enable)
{
    static const int green_led_pin = 17;
    Logger.Log(QString("Green led switched: %1").arg(enable));
    ModbusMaster::ModbusError modbus_write_green_led_answer = m_modbus_computer->writeBit(green_led_pin, enable);
    if (modbus_write_green_led_answer != ModbusMaster::ModbusError::NO_ERROR)
    {
        Logger.Log(ModbusMaster::ModbusErrorText[modbus_write_green_led_answer]);
        emit showErrorDialog(ModbusMaster::ModbusErrorText[modbus_write_green_led_answer]);
    }
}

void Core::syncSettings()
{
    if (!m_settings->contains("general/serial_number"))
        m_settings->setValue("general/serial_number", 2304);

    if (!m_settings->contains("general/controller_version"))
        m_settings->setValue("general/controller_version", 1);

    if (!m_settings->contains("general/com_port"))
        m_settings->setValue("general/com_port", 3);

    if (!m_settings->contains("general/baud_rate"))
        m_settings->setValue("general/baud_rate", 19200);

    if (!m_settings->contains("general/stop_bits"))
        m_settings->setValue("general/stop_bits", 1);

    if (!m_settings->contains("general/data_bits"))
        m_settings->setValue("general/data_bits", 8);

    if (!m_settings->contains("general/parity"))
        m_settings->setValue("general/parity", "No parity");

    if (!m_settings->contains("general/response_time"))
        m_settings->setValue("general/response_time", 1500);

    if (!m_settings->contains("general/address"))
        m_settings->setValue("general/address", 1);

    if (!m_settings->contains("general/polling_interval_ms"))
        m_settings->setValue("general/polling_interval_ms", 1000);

    if (!m_settings->contains("general/wnd_launch_timeout"))
        m_settings->setValue("general/wnd_launch_timeout", 2000);

    if (!m_settings->contains("general/use_wnd_resize"))
        m_settings->setValue("general/use_wnd_resize", false);

    if (!m_settings->contains("general/default_language"))
        m_settings->setValue("general/default_language", "RU");

    QStringList probationTypes = m_settings->childGroups();
    probationTypes.removeOne("General");
    QSet<QString> existingTypes;
    for (const QString& it : probationTypes) {
        if (m_settings->contains(QString("%1/name").arg(it))) {
            existingTypes.insert(m_settings->value(QString("%1/name").arg(it)).toString());
        }
    }

    static constexpr char ImageFolder[] = "/schemes";
    for (const QFileInfo& dirInfo : QDir(QDir::currentPath() + ImageFolder).entryInfoList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot))
    {
        const auto base = dirInfo.baseName();
        if (!existingTypes.contains(base)) {
            auto newType = QUuid::createUuid().toString();
            m_settings->setValue(QString("%1/exe").arg(newType), "");
            m_settings->setValue(QString("%1/name").arg(newType), dirInfo.baseName());
            m_settings->setValue(QString("%1/modbus_outputs").arg(newType), QStringList() << "1" << "2" << "3");
            m_settings->setValue(QString("%1/modbus_inputs").arg(newType), QStringList() << "3" << "2" << "1");

            int i = 0;
            m_settings->beginWriteArray(QString("%1/probations").arg(newType));
            for (const QFileInfo& picInfo : QDir(dirInfo.absoluteFilePath()).entryInfoList(QStringList() << "*.jpg" << "*.jpeg" << "*.png", QDir::Files))
            {
                m_settings->setArrayIndex(i++);
                m_settings->setValue("name", picInfo.baseName());
                m_settings->setValue("image_path", QString(".%1/%2/%3.%4").arg(ImageFolder, dirInfo.baseName(), picInfo.baseName(),picInfo.suffix()));
                m_settings->setValue("hint", "");
            }
            m_settings->endArray();
        }
    }

    m_settings->sync();
}

void Core::loadProbationData()
{
    m_probation_data.clear();
    QStringList existingTypes = m_settings->childGroups();
    existingTypes.removeOne("General");

    for (const auto &item : existingTypes) {
        QString title = m_settings->value(QString("%1/name").arg(item)).toString();

        QBitArray modbus_inputs(16, false);
        auto inputs = m_settings->value(QString("%1/modbus_inputs").arg(item)).toStringList();
        for (const auto &i : inputs) {
            int bit = i.toInt() - input_register_adress * 16;
            if (bit < 0 || bit > 15)
                throw std::runtime_error(QString("error in %1").arg(QString("%1/modbus_inputs").arg(item)).toUtf8().data());
            modbus_inputs.setBit(bit);
        }

        QBitArray modbus_outputs(16, false);
        auto outputs = m_settings->value(QString("%1/modbus_outputs").arg(item)).toStringList();
        for (const auto &i : outputs) {
            int bit = i.toInt() - output_register_adress * 16;
            if (bit < 0 || bit > 15)
                throw std::runtime_error(QString("error in %1").arg(QString("%1/modbus_inputs").arg(item)).toUtf8().data());
            modbus_outputs.setBit(bit);
        }

        QVariantList schemes;
        int ssize = m_settings->beginReadArray(QString("%1/probations").arg(item));
        for(int i = 0; i < ssize; ++i) {
            m_settings->setArrayIndex(i);
            ProbationScheme scheme;
            scheme.title = m_settings->value("name").toString();
            scheme.hint = m_settings->value("hint").toString();
            scheme.scheme = QPixmap(m_settings->value("image_path").toString());
            schemes << QVariant::fromValue(scheme);
        }
        m_settings->endArray();

        ProbationType ptype;
        ptype.title = title;
        ptype.icon = kTestIcon.value(title);
        ptype.aux_path = m_settings->value(QString("%1/exe").arg(item)).toString();
        ptype.modbus_inputs = std::move(modbus_inputs);
        ptype.modbus_outputs = std::move(modbus_outputs);
        ptype.schemes = std::move(schemes);

        m_probation_data[title] = QVariant::fromValue(ptype);
    }
}

bool Core::setupCommandMode()
{
    QPair<ModbusMaster::Status, ModbusMaster::ModbusError> modbus_status = m_modbus_computer->readStatus();
    if (modbus_status.second != ModbusMaster::ModbusError::NO_ERROR)
        return false;

    auto err = ModbusMaster::ModbusError::NO_ERROR;
    if (modbus_status.first != ModbusMaster::s_MANUAL)
        err = m_modbus_computer->writeCommand(ModbusMaster::c_MANUAL);
    return err == ModbusMaster::ModbusError::NO_ERROR;
}

void Core::switchVoltageWarning(const QBitArray &output_register_data)
{
    static const int red_led_pin = 16;
    bool enabled = output_register_data[red_led_pin - output_register_adress * 16];
    Logger.Log(QString("Voltage %1").arg(enabled ? "enabled" : "disabled"));
    emit voltageSwitched(enabled);
}
