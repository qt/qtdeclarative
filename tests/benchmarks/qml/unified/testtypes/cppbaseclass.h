// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CPPBASECLASS_H
#define CPPBASECLASS_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqml.h>

class CppBaseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cppProp MEMBER cppProp BINDABLE cppPropBindable)
    Q_PROPERTY(int bProp MEMBER bProp BINDABLE bPropBindable)

    Q_PROPERTY(int d READ d WRITE setD NOTIFY dChanged)
    Q_PROPERTY(int e READ e WRITE setE NOTIFY eChanged)

    QML_ELEMENT
public:
    QProperty<int> cppProp;
    QProperty<int> bProp;
    QBindable<int> cppPropBindable() { return QBindable<int>(&cppProp); }
    QBindable<int> bPropBindable() { return QBindable<int>(&bProp); }

    Q_INVOKABLE void doCall(QObject *foo);

    int d() const
    {
        return m_d;
    }

    void setD(int newD)
    {
        if (m_d == newD)
            return;
        m_d = newD;
        emit dChanged();
    }

    int e() const
    {
        return m_e;
    }

    void setE(int newE)
    {
        if (m_e == newE)
            return;
        m_e = newE;
        emit eChanged();
    }

signals:
    void dChanged();
    void eChanged();

private:
    int m_d = 0;
    int m_e = 0;
};

inline void CppBaseClass::doCall(QObject *foo)
{
    cppProp = foo ? 17 : 18;
}

#endif // CPPBASECLASS_H
