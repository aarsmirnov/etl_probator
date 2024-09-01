#include "logfile.h"

#include <stdexcept>

#include <QDir>
#include <QTextCodec>
#include <QDateTime>
#include <QBitArray>

QString BitsToString(const QBitArray& bits)
{
    QString result;
    for (int i = 0; i < bits.size(); ++i) {
        result.append(bits[i] ? "1" : "0");
    }
    return result;
}

LoggerClass::LoggerClass()
    :  m_file(QDir::currentPath() + "/logfile.txt")
{
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append))
        throw std::runtime_error(QObject::tr("Ошибка открытия файла логирования").toUtf8().data());
}

LoggerClass::~LoggerClass()
{
    Log(QObject::tr("Конец сеанса"));
    m_file.close();
}

void LoggerClass::Log(const QString &message)
{
    QTextStream stream(&m_file);
    stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ")
           << ": "
           << message.toUtf8() << "\n";
    stream.flush();
}
