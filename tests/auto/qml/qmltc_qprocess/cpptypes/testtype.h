// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTTYPE_H
#define TESTTYPE_H

#include <QtQmlIntegration/qqmlintegration.h>
#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>
#include <QtGui/qfont.h>

class TypeWithVersionedAlias : public QObject
{
    Q_OBJECT
    QML_UNCREATABLE("")
    QML_ELEMENT
    QString m_readAndWrite;

public:
    TypeWithVersionedAlias() { }
    Q_PROPERTY(QString notExisting MEMBER m_readAndWrite REVISION(6, 0));
    Q_PROPERTY(QString existing MEMBER m_readAndWrite REVISION(1, 0));
};

class UncreatableType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
};

class NoDefaultConstructorType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    NoDefaultConstructorType() = delete;
};

class SingletonType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
};

class NotSingletonType : public SingletonType
{
    Q_OBJECT
    QML_ELEMENT
};

class NormalTypeAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    NormalTypeAttached(QObject* parent): QObject(parent) {}
};

class NormalType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(NormalTypeAttached)

    static NormalTypeAttached *qmlAttachedProperties(QObject *object) {
        return new NormalTypeAttached(object);
    }
};

class TypeWithSignals : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
Q_SIGNALS:
    void signalWithConstPointerToGadget(const QFont *); // not allowed
    void signalWithConstPointerToGadgetConst(const QFont *const); // not allowed
    void signalWithPointerToGadgetConst(QFont *const); // not allowed
    void signalWithPointerToGadget(QFont *); // not allowed

    void signalWithPrimitivePointer(int *);
    void signalWithConstPrimitivePointer(const int *);
};

struct UnknownValueType
{
    Q_GADGET

private:
    int a = 12;
    friend bool operator==(const UnknownValueType &a, const UnknownValueType &b)
    {
        return a.a == b.a;
    }

    friend bool operator!=(const UnknownValueType &a, const UnknownValueType &b)
    {
        return !(a == b);
    }
};

class TypeWithUnknownPropertyType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(UnknownValueType u READ u WRITE setU NOTIFY uChanged)

public:
    UnknownValueType u() const { return m_u; }
    void setU(const UnknownValueType &u)
    {
        if (u != m_u) {
            m_u = u;
            emit uChanged();
        }
    }

signals:
    void uChanged();

private:
    UnknownValueType m_u;
};

#endif // TESTTYPE_H
