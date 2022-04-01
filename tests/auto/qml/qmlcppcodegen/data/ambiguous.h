#ifndef AMBIGUOUS_H
#define AMBIGUOUS_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class Ambiguous : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 2)

public:
    Ambiguous(QObject *parent = nullptr) : QObject(parent)
    {
        setObjectName(QStringLiteral("Ambiguous"));
    }
};

#endif // AMBIGUOUS_H
