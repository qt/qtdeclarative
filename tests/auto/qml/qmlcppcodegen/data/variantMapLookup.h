#pragma once
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class VariantMapLookupFoo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap data READ data CONSTANT)
    Q_PROPERTY(QList<QVariantMap> many READ many CONSTANT)

public:
    VariantMapLookupFoo(QObject *parent = nullptr) : QObject(parent) { }

private:
    QVariantMap data() const { return { { QStringLiteral("value"), 42 } }; }
    QList<QVariantMap> many() const
    {
        const QVariantMap one = data();
        return QList<QVariantMap>({one, one, one});
    }
};


