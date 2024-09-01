#pragma once

#include <QFile>
#include <QTextStream>

QString BitsToString(const QBitArray& bits);

class LoggerClass
{
private:
    LoggerClass();
    LoggerClass(const LoggerClass&) = delete;
    LoggerClass& operator=(const LoggerClass&) = delete;
    ~LoggerClass();

public:
    static LoggerClass& LoggerInstance()
    {
        static LoggerClass instance;
        return instance;
    }

    void Log(const QString &message);

private:
    QFile m_file;
};

#define Logger LoggerClass::LoggerInstance()
