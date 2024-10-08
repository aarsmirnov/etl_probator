#ifndef DEVICE_H
#define DEVICE_H

#include <QWidget>
#include <QVariantMap>

class Device : public QWidget
{
    Q_OBJECT

public:
    explicit Device(QWidget *parent = nullptr);
    explicit Device(const QString &title, const QPixmap &schema, QWidget *parent = nullptr);
    ~Device();

    virtual QString name() const = 0;

    virtual void setParams(const QVariantMap &params);
    virtual QVariantMap getResult();
    virtual QVector<QStringList> protocol();

protected:
    QString title() const;
    QPixmap schema() const;

    void pushEvent(const QString &text);

signals:
    void eventOccured(const QString &text);

private:
    QString     m_title;
    QPixmap     m_schema;
};

#endif // DEVICE_H
