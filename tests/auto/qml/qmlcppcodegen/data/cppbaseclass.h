// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CPPBASECLASS_H
#define CPPBASECLASS_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqml.h>

class CppBaseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cppProp MEMBER cppProp BINDABLE cppPropBindable FINAL)
    Q_PROPERTY(int cppProp2 MEMBER cppProp2 BINDABLE cppProp2Bindable FINAL)
    Q_PROPERTY(QList<int> boo MEMBER boo FINAL CONSTANT)
    Q_PROPERTY(QList<qreal> hoo MEMBER hoo FINAL CONSTANT)
    Q_PROPERTY(int inaccessible READ inaccessible FINAL CONSTANT REVISION(1, 5))
    QML_ADDED_IN_VERSION(1, 0)
    QML_ELEMENT
public:
    CppBaseClass(QObject *parent = nullptr)
        : QObject(parent)
    {
        boo.append(16);
        boo.append(17);

        hoo.append(0.25);
        hoo.append(13.5);
    }

    QProperty<int> cppProp;
    QBindable<int> cppPropBindable() { return QBindable<int>(&cppProp); }

    QProperty<int> cppProp2;
    QBindable<int> cppProp2Bindable() { return QBindable<int>(&cppProp2); }

    Q_INVOKABLE void doCall(QObject *foo);

    int inaccessible() const { return 7; }
private:
    QList<int> boo;
    QList<qreal> hoo;
};

inline void CppBaseClass::doCall(QObject *foo)
{
    cppProp = foo ? 17 : 18;
}

class CppSingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CppSingleton(QObject *parent = nullptr) : QObject(parent)
    {
        setObjectName(QStringLiteral("ItIsTheSingleton"));
    }
};

#endif // CPPBASECLASS_H
