// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SINGLETONTYPE_H
#define SINGLETONTYPE_H

#include <QObject>
#include <qqml.h>

class SingletonType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int mySingletonValue READ mySingletonValue WRITE setMySingletonValue NOTIFY
                       mySingletonValueChanged)
    int m_mySingletonValue = 42;

public:
    explicit SingletonType(QObject *parent = nullptr);
    static SingletonType *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        SingletonType *result = new SingletonType(nullptr);
        return result;
    }
    int mySingletonValue() { return m_mySingletonValue; }

    void setMySingletonValue(int newValue)
    {
        if (m_mySingletonValue != newValue) {
            m_mySingletonValue = newValue;
            emit mySingletonValueChanged();
        }
    }

    enum MyCppEnum { Hello, Enum };
    Q_ENUM(MyCppEnum)

signals:
    void mySingletonValueChanged();
};

#endif // SINGLETONTYPE_H
