#include "device.h"

Device::Device(QWidget *parent)
    : QWidget(parent)
{

}

Device::Device(const QString &title, const QPixmap &schema, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_schema(schema)
{

}

Device::~Device()
{

}

void Device::setParams(const QVariantMap &params)
{
    Q_UNUSED(params)
}

QVariantMap Device::getResult()
{
    return QVariantMap{};
}

QVector<QStringList> Device::protocol()
{
    return QVector<QStringList>();
}

QString Device::title() const
{
    return m_title;
}

QPixmap Device::schema() const
{
    return m_schema;
}

void Device::pushEvent(const QString &text)
{
    emit eventOccured(text);
}
