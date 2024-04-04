// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef EXTENSIONTYPES_H
#define EXTENSIONTYPES_H

#include <QtCore/qobject.h>
#include <qqml.h>
#include <QtQml/qqmlparserstatus.h>

class Extension : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ getCount WRITE setCount NOTIFY countChanged)

public:
    Extension(QObject *parent = nullptr) : QObject(parent) { }
    int getCount() const { return 42; }
    void setCount(int) { }

    enum EnumFromExtension {
        ThisIsTheEnumFromExtension,
    };
    Q_ENUM(EnumFromExtension)
    enum FlagFromExtension {
        ThisIsTheFlagFromExtension,
    };
    Q_DECLARE_FLAGS(FlagsFromExtension, FlagFromExtension)
    Q_FLAG(FlagsFromExtension)

Q_SIGNALS:
    void countChanged();
};

class IndirectExtension : public Extension
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    IndirectExtension(QObject *parent = nullptr) : Extension(parent) { }
};

class Extended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension)
    Q_PROPERTY(double count READ getCount WRITE setCount NOTIFY countChanged)

public:
    Extended(QObject *parent = nullptr) : QObject(parent) { }
    double getCount() const { return 0.0; }
    void setCount(double) { }

    enum EnumFromExtended {
        ThisIsTheEnumFromExtended,
    };
    Q_ENUM(EnumFromExtended)
    enum FlagFromExtended {
        ThisIsTheFlagFromExtended,
    };
    Q_DECLARE_FLAGS(FlagsFromExtended, FlagFromExtended)
    Q_FLAG(FlagsFromExtended)
Q_SIGNALS:
    void countChanged();
};

class ExtendedIndirect : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(IndirectExtension)
    Q_PROPERTY(double count READ getCount WRITE setCount NOTIFY countChanged)

public:
    ExtendedIndirect(QObject *parent = nullptr) : QObject(parent) { }
    double getCount() const { return 0; }
    void setCount(double) { }
Q_SIGNALS:
    void countChanged();
};

class Extension2 : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString str READ getStr WRITE setStr NOTIFY strChanged)

public:
    Extension2(QObject *parent = nullptr) : QObject(parent) { }
    QString getStr() const { return QStringLiteral("42"); }
    void setStr(QString) { }
Q_SIGNALS:
    void strChanged();
};

class ExtendedTwice : public Extended
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension2)
    Q_PROPERTY(QByteArray str READ getStr WRITE setStr)

public:
    ExtendedTwice(QObject *parent = nullptr) : Extended(parent) { }
    QByteArray getStr() const { return QByteArray(); }
    void setStr(QByteArray) { }
};

class AttachedType : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    AttachedType(QObject *parent = nullptr) : QObject(parent) { }
};

class ExtensionNamespace : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ getCount WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(QObject *p READ p CONSTANT)

    Q_CLASSINFO("DefaultProperty", "objectName")
    Q_CLASSINFO("ParentProperty", "p")

    Q_INTERFACES(QQmlParserStatus)
    QML_ATTACHED(AttachedType)

    QObject *m_p = nullptr;

public:
    ExtensionNamespace(QObject *parent = nullptr) : QObject(parent), m_p(parent) { }
    int getCount() const { return 42; }
    void setCount(int) { }

    QObject *p() const { return m_p; }

    enum ExtensionEnum {
        Value1,
        Value2,
    };
    Q_ENUM(ExtensionEnum)

    Q_INVOKABLE int someMethod() { return 42; }

    void classBegin() override { }
    void componentComplete() override { }

    static AttachedType *qmlAttachedProperties(QObject *parent) { return new AttachedType(parent); }

Q_SIGNALS:
    void countChanged();
};

class NamespaceExtended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED_NAMESPACE(ExtensionNamespace)
};

class NonNamespaceExtended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(ExtensionNamespace)
};

#endif // EXTENSIONTYPES_H
