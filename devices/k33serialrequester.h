#ifndef K33SERIALREQUESTER_H
#define K33SERIALREQUESTER_H

#include <QSerialPort>

class K33SerialRequester : public QObject
{
    Q_OBJECT
public:
    explicit K33SerialRequester(const QString &port, QObject *parent = nullptr);

    bool open();
    void close();
    bool isOpen();

    void writeRequest(const QByteArray &data);


signals:
    void dataReceived(const QByteArray &data);

private:
    void onNewReceivedData();

private:
    QSerialPort     *m_serialPort;

    QByteArray       m_response;
    int              m_toReadBytes { 0 };
    bool             m_hasStart { false };
};

#endif // K33SERIALREQUESTER_H
