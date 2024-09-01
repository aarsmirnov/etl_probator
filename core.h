#pragma once

#include <QObject>
#include <QVariant>
#include <QBitArray>
#include <QPixmap>
#include <QSet>


QT_FORWARD_DECLARE_CLASS(QSettings);
class ModbusMaster;

struct ProbationScheme
{
    QString title;
    QString icon;
    QPixmap scheme;
    QString hint;
};
Q_DECLARE_METATYPE(ProbationScheme);

struct ProbationType
{
    QString         title;
    QString         icon;
    QVariantList    schemes;
    QBitArray       modbus_inputs;
    QBitArray       modbus_outputs;
    QString         aux_path;
};
Q_DECLARE_METATYPE(ProbationType);


class Core: public QObject
{
    Q_OBJECT

public:
    explicit Core(QObject *parent = nullptr);
    ~Core();

    QVariant generalSetting(const QString& key) const;

    QVariantMap getProbationData() const;

    void setupProbation(const QBitArray& outputs);

    enum Blocks
    {
        VOLTAGE = 6,
        CURRENT = 7,
        DOORS = 5,
        EMERGENCY_BUTTON = 4,
        GND = 2,
        WORK_GND = 3,
        BLOCKS_COUNT = CURRENT + 1
    };
    Q_ENUM(Blocks)

    static bool BelongsToBlocks(int i)
    {
        static const QSet<int> vals = {6, 7, 5, 4, 2, 3};
        return vals.contains(i);
    }

    static QMap<Blocks, QString> IndicationErrors;

signals:
    void showErrorDialog(const QString &message);

    void switchIndication(const QBitArray&);

    void voltageSwitched(bool);

public slots:
    void poller();

    void start();

    void stop();

    void switchGreenLED(bool enable);

private:
    void syncSettings();

    void loadProbationData();

    bool setupCommandMode();

    void switchVoltageWarning(const QBitArray &output_register_data);

private:
    const int           output_register_adress = 1;
    const int           input_register_adress = 0;

    QSettings*          m_settings;
    ModbusMaster*       m_modbus_computer;
    QTimer*             m_polling_timer;

    QVariantMap         m_probation_data;
    QString             m_default_language;
    QString             m_excel_protocol;
    QBitArray           m_modbus_outputs = QBitArray(16, false);
};
