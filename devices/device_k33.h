#ifndef DEVICE_K33_H
#define DEVICE_K33_H

#include <QWidget>
#include <QStandardItemModel>

#include "device.h"
#include "k33serialrequester.h"
#include "k33serialpackets.h"

namespace Ui {
class Device_K33;
}

class Device_K33 : public Device
{
    Q_OBJECT

public:
    explicit Device_K33(const QString &title, const QPixmap &schema, Device *parent = nullptr);
    ~Device_K33();

    QString name() const override { return QStringLiteral("Коэффициент 3.3"); }
    QVector<QStringList> protocol() override;

private:
    void configUi();
    void setParamMode(bool enable);
    void updateMeasureResult();
    void updateKtMeasureResult();
    void updateHhMeasureResult();
    void updateKzMeasureResult();

    void insertProtocolRecords();
    void insertKtProtocolRecords();
    void insertHhProtocolRecords();
    void insertKzProtocolRecords();
    void removeProtocolRecords();
    QStandardItemModel *getCurrentProtocolModel();

private:
    Ui::Device_K33 *ui;

    K33SerialRequester          *m_serialRequester { nullptr };
    bool                         m_readyToMeasure { false };

    QMap<int, QMap<QString, QString>>       m_measureResult;

    QStandardItemModel          *m_terminalModel;
    QStandardItemModel          *m_ktProtocolModel;
    QStandardItemModel          *m_hhProtocolModel;
    QStandardItemModel          *m_kzProtocolModel;
};

#endif // DEVICE_K33_H
