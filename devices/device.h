#ifndef DEVICE_H
#define DEVICE_H

#include <QWidget>
#include <QVariantMap>

class Device : public QWidget
{
    Q_OBJECT

public:
    explicit Device(QWidget *parent = nullptr);
    explicit Device(const QString &title, QWidget *parent = nullptr);
    ~Device();

    virtual QString name() const = 0;

    virtual void setParams(const QVariantMap &params);
    virtual QVariantMap getResult();

protected:
    QString title() const;

signals:
    void eventOccured(const QString &text);

private:
    QString     m_title;
};

#endif // DEVICE_H
