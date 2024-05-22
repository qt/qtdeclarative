#pragma once
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class WritableVariantMap : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap data READ data WRITE setData NOTIFY dataChanged)

public:
    WritableVariantMap(QObject *parent = nullptr) : QObject(parent) { }

    QVariantMap data() const { return m_data; }
    void setData(const QVariantMap &data)
    {
        if (m_data != data) {
            m_data = data;
            emit dataChanged();
        }
    }

signals:
    void dataChanged();

private:
    QVariantMap m_data;
};


