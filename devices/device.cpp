#include "device.h"

Device::Device(QWidget *parent)
    : QWidget(parent)
{

}

Device::Device(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
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

QString Device::title() const
{
    return m_title;
}
