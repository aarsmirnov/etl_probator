#include "utils.h"

#include <QFile>

QString utils::loadStyleSheet(const QString &path)
{
    QFile styleFile(path);
    if (!styleFile.open(QFile::ReadOnly)) {
        return QString{};
    }
    return styleFile.readAll();
}
