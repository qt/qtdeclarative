// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtQml/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

//![0]
// Singleton.h
class Singleton : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int thing READ thing WRITE setThing NOTIFY thingChanged FINAL)
    QML_ELEMENT
    QML_SINGLETON

public:
    Singleton(QObject *parent = nullptr) : QObject(parent) {}

    int thing() const { return m_value; }
    void setThing(int v)
    {
        if (v != m_value) {
            m_value = v;
            emit thingChanged();
        }
    }

signals:
    void thingChanged();

private:
    int m_value = 12;
};
//![0]

inline void setTheThing(QQmlEngine *engine)
{
//![1]
    Singleton *singleton
            = engine->singletonInstance<Singleton *>("MyModule", "Singleton");
    singleton->setThing(77);
//![1]
}

#endif
