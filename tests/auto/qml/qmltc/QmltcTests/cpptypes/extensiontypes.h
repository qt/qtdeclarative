// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef EXTENSIONTYPES_H
#define EXTENSIONTYPES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <qqml.h>

class Extension : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)
    Q_PROPERTY(double foo READ getFoo WRITE setFoo BINDABLE bindableFoo)
    Q_PROPERTY(QQmlListProperty<QObject> myList READ getMyList)

    QProperty<int> m_extCount { 0 };
    QProperty<double> m_foo { 0 };
    QList<QObject *> m_myList;

public:
    Extension(QObject *parent = nullptr);
    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();

    double getFoo() const;
    void setFoo(double v);
    QBindable<double> bindableFoo();

    QQmlListProperty<QObject> getMyList();
};

class IndirectExtension : public Extension
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    IndirectExtension(QObject *parent = nullptr);
};

class TypeWithExtension : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)
    QML_EXTENDED(Extension)

    QProperty<int> m_count;

public:
    TypeWithExtension(QObject *parent = nullptr);
    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();

    static constexpr int unsetCount = 100;
};

class Extension2 : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString str READ getStr WRITE setStr BINDABLE bindableStr)

    QProperty<QString> m_extStr {};

public:
    Extension2(QObject *parent = nullptr);

    QString getStr() const;
    void setStr(QString v);
    QBindable<QString> bindableStr();
};

class TypeWithExtensionDerived : public TypeWithExtension
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString str READ getStr WRITE setStr BINDABLE bindableStr)
    QML_EXTENDED(Extension2)

    QProperty<QString> m_str;

public:
    TypeWithExtensionDerived(QObject *parent = nullptr);

    QString getStr() const;
    void setStr(QString v);
    QBindable<QString> bindableStr();

    static const QString unsetStr;
};

class TypeWithExtensionNamespace : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)
    QML_EXTENDED_NAMESPACE(Extension)

    QProperty<int> m_count;

public:
    TypeWithExtensionNamespace(QObject *parent = nullptr);

    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();
};

class TypeWithBaseTypeExtension : public TypeWithExtensionDerived
{
    Q_OBJECT
    QML_ELEMENT

public:
    TypeWithBaseTypeExtension(QObject *parent = nullptr) : TypeWithExtensionDerived(parent) { }
};

#endif // EXTENSIONTYPES_H
