// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TYPES_H
#define TYPES_H

#include <QObject>
#include <QQmlListProperty>

class Type : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int baseProperty MEMBER m_baseProperty)

public:
    Type(QObject *parent = nullptr)
        : QObject(parent) {}

private:
    int m_baseProperty;
};

class ExtendedType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int extendedProperty MEMBER m_extendedProperty)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    ExtendedType(QObject *parent = nullptr)
        : QObject(parent) {}
    QQmlListProperty<QObject> data() { return QQmlListProperty<QObject>(this, &m_data); }

private:
    QList<QObject *> m_data;
    int m_extendedProperty;
};

class DerivedType1 : public Type
{
    Q_OBJECT
    Q_PROPERTY(int m_exendedProperty2 MEMBER m_extendedProperty2)

public:
    DerivedType1(QObject *parent = nullptr)
        : Type(parent) {}

private:
    int m_extendedProperty2;
};

class DerivedType2 : public DerivedType1
{
    Q_OBJECT
public:
    DerivedType2(QObject *parent = nullptr)
        : DerivedType1(parent) {}
};

#endif // TYPES_H
