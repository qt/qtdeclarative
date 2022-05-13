// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTPRIVATEPROPERTY_H
#define TESTPRIVATEPROPERTY_H

#include "testgroupedtype.h"

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <qqml.h>

#include <private/qobject_p.h>

class ValueTypeGroup
{
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(int count READ count WRITE setCount)

    int m_count;

public:
    int count() const { return m_count; }
    void setCount(int c) { m_count = c; }
};

class PrivatePropertyTypePrivate;
class PrivatePropertyType : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PRIVATE_PROPERTY(PrivatePropertyType::d_func(), ValueTypeGroup vt READ vt WRITE setVt)
    Q_PRIVATE_PROPERTY(PrivatePropertyType::d_func(),
                       int smth READ smth WRITE setSmth BINDABLE bindableSmth)
    Q_PRIVATE_PROPERTY(PrivatePropertyType::d_func(),
                       QString foo READ foo WRITE setFoo NOTIFY fooChanged)
    Q_PRIVATE_PROPERTY(PrivatePropertyType::d_func(), TestTypeGrouped *group READ getGroup)

    Q_DECLARE_PRIVATE(PrivatePropertyType)
public:
    PrivatePropertyType(QObject *parent = nullptr);
signals:
    void fooChanged();
};

class PrivatePropertyTypePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(PrivatePropertyType)

    QProperty<int> m_smth;
    QProperty<ValueTypeGroup> m_vt;
    QString m_foo;
    TestTypeGrouped m_group;

public:
    static PrivatePropertyTypePrivate *get(PrivatePropertyType *q) { return q->d_func(); }
    static const PrivatePropertyTypePrivate *get(const PrivatePropertyType *q)
    {
        return q->d_func();
    }

    ValueTypeGroup vt() const;
    void setVt(ValueTypeGroup vt);

    int smth() const;
    void setSmth(int s);
    QBindable<int> bindableSmth();

    QString foo() const;
    void setFoo(const QString &foo);

    TestTypeGrouped *getGroup();
};

#endif // TESTPRIVATEPROPERTY_H
