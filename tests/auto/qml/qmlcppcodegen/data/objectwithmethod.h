// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef OBJECTWITHMETOD_H
#define OBJECTWITHMETOD_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqml.h>
#include <QtQml/private/qv4engine_p.h>

// Make objectName available. It doesn't exist on the builtin QtObject type
struct QObjectForeignForObjectName {
    Q_GADGET
    QML_FOREIGN(QObject)
    QML_ANONYMOUS
};

class ObjectWithMethod : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int fff MEMBER theThing BINDABLE theThingBindable FINAL)

public:
    ObjectWithMethod(QObject *parent = nullptr) : QObject(parent) { theThing = 5; }

    Q_INVOKABLE int doThing() const { return theThing; }
    QProperty<int> theThing;
    QBindable<int> theThingBindable() { return QBindable<int>(&theThing); }

    Q_INVOKABLE void overloaded(QQmlV4Function *) { setObjectName(QStringLiteral("javaScript")); }
    Q_INVOKABLE void overloaded(double) { setObjectName(QStringLiteral("double")); }
    Q_INVOKABLE void overloaded(const QString &) { setObjectName(QStringLiteral("string")); }

    Q_INVOKABLE void foo(const QString &bla) { setObjectName(bla); }
    Q_INVOKABLE void foo(ObjectWithMethod *) { setObjectName(QStringLiteral("ObjectWithMethod")); }
};

class OverriddenObjectName : public ObjectWithMethod
{
    Q_OBJECT
    Q_PROPERTY(QString objectName READ objectName WRITE setObjectName BINDABLE objectNameBindable)

    // This shouldn't work
    Q_PROPERTY(int fff READ fff BINDABLE nothingBindable)

public:
    OverriddenObjectName(QObject *parent = nullptr) : ObjectWithMethod(parent)
    {
        m_objectName = QStringLiteral("borschtsch");
        nothing = 77;
    }

    QString objectName() const { return m_objectName.value(); }
    void setObjectName(const QString &objectName) { m_objectName.setValue(objectName); }
    QBindable<QString> objectNameBindable() { return QBindable<QString>(&m_objectName); }
    Q_INVOKABLE QString doThing() const { return QStringLiteral("7"); }

    int fff() const { return nothing.value(); }
    QBindable<int> nothingBindable() { return QBindable<int>(&nothing); }
private:
    QProperty<int> nothing;
    QProperty<QString> m_objectName;
};

class ObjectWithStringListMethod : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ObjectWithStringListMethod(QObject *parent = nullptr) : QObject(parent)
    {
        m_names.append("One");
        m_names.append("Two");
    }

    Q_INVOKABLE QStringList names() const { return m_names; }

private:
    QStringList m_names;
};

class ObjectFactory : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ObjectFactory(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE ObjectWithStringListMethod *getFoo()
    {
        if (!m_foo)
            m_foo = new ObjectWithStringListMethod(this);
        return m_foo;
    }

private:
    ObjectWithStringListMethod *m_foo = nullptr;
};

#endif // OBJECTWITHMETHOD_H
