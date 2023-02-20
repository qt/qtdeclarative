#pragma once
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class VariantMapLookupFoo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap data READ data CONSTANT)

public:
    VariantMapLookupFoo(QObject *parent = nullptr) : QObject(parent) { }

private:
    QVariantMap data() const { return { { "value", 42 } }; }
};
