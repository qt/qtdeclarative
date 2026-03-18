// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CPPTYPES_H
#define CPPTYPES_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

// ============================================================================
// C++ BASE TYPES — each exposes "value" returning 1
// ============================================================================

class CppPlainBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ value CONSTANT)
public:
    using QObject::QObject;
    int value() const { return 1; }
};

class CppVirtualBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ value CONSTANT VIRTUAL)
public:
    using QObject::QObject;
    int value() const { return 1; }
};

class CppFinalBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ value CONSTANT FINAL)
public:
    using QObject::QObject;
    int value() const { return 1; }
};

class CppOverrideBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ value CONSTANT OVERRIDE)
public:
    using QObject::QObject;
    int value() const { return 1; }
};

class CppMethodBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;
    Q_INVOKABLE int value() const { return 1; }
};

// ============================================================================
// C++ DERIVED: plain property shadows — each returns 2
// ============================================================================

class CppBare_CppPlain : public CppPlainBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT)
public:
    using CppPlainBase::CppPlainBase;
    int derivedValue() const { return 2; }
};

class CppBare_CppVirtual : public CppVirtualBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT)
public:
    using CppVirtualBase::CppVirtualBase;
    int derivedValue() const { return 2; }
};

class CppBare_CppFinal : public CppFinalBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT)
public:
    using CppFinalBase::CppFinalBase;
    int derivedValue() const { return 2; }
};

class CppBare_CppMethod : public CppMethodBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT)
public:
    using CppMethodBase::CppMethodBase;
    int derivedValue() const { return 2; }
};

class CppBare_CppOverride : public CppOverrideBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT)
public:
    using CppOverrideBase::CppOverrideBase;
    int derivedValue() const { return 2; }
};

// ============================================================================
// C++ DERIVED: VIRTUAL property — each returns 2
// ============================================================================

class CppVirtual_CppPlain : public CppPlainBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT VIRTUAL)
public:
    using CppPlainBase::CppPlainBase;
    int derivedValue() const { return 2; }
};

class CppVirtual_CppVirtual : public CppVirtualBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT VIRTUAL)
public:
    using CppVirtualBase::CppVirtualBase;
    int derivedValue() const { return 2; }
};

class CppVirtual_CppFinal : public CppFinalBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT VIRTUAL)
public:
    using CppFinalBase::CppFinalBase;
    int derivedValue() const { return 2; }
};

class CppVirtual_CppMethod : public CppMethodBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT VIRTUAL)
public:
    using CppMethodBase::CppMethodBase;
    int derivedValue() const { return 2; }
};

class CppVirtual_CppOverride : public CppOverrideBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT VIRTUAL)
public:
    using CppOverrideBase::CppOverrideBase;
    int derivedValue() const { return 2; }
};

// ============================================================================
// C++ DERIVED: OVERRIDE property — each returns 2
// ============================================================================

class CppOverride_CppPlain : public CppPlainBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT OVERRIDE)
public:
    using CppPlainBase::CppPlainBase;
    int derivedValue() const { return 2; }
};

class CppOverride_CppVirtual : public CppVirtualBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT OVERRIDE)
public:
    using CppVirtualBase::CppVirtualBase;
    int derivedValue() const { return 2; }
};

class CppOverride_CppFinal : public CppFinalBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT OVERRIDE)
public:
    using CppFinalBase::CppFinalBase;
    int derivedValue() const { return 2; }
};

class CppOverride_CppMethod : public CppMethodBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT OVERRIDE)
public:
    using CppMethodBase::CppMethodBase;
    int derivedValue() const { return 2; }
};

class CppOverride_CppOverride : public CppOverrideBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT OVERRIDE)
public:
    using CppOverrideBase::CppOverrideBase;
    int derivedValue() const { return 2; }
};

// ============================================================================
// C++ DERIVED: FINAL property — each returns 2
// ============================================================================

class CppFinal_CppPlain : public CppPlainBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT FINAL)
public:
    using CppPlainBase::CppPlainBase;
    int derivedValue() const { return 2; }
};

class CppFinal_CppVirtual : public CppVirtualBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT FINAL)
public:
    using CppVirtualBase::CppVirtualBase;
    int derivedValue() const { return 2; }
};

class CppFinal_CppFinal : public CppFinalBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT FINAL)
public:
    using CppFinalBase::CppFinalBase;
    int derivedValue() const { return 2; }
};

class CppFinal_CppMethod : public CppMethodBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT FINAL)
public:
    using CppMethodBase::CppMethodBase;
    int derivedValue() const { return 2; }
};

class CppFinal_CppOverride : public CppOverrideBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ derivedValue CONSTANT FINAL)
public:
    using CppOverrideBase::CppOverrideBase;
    int derivedValue() const { return 2; }
};

// ============================================================================
// C++ DERIVED: method shadows property/method — each returns 2
// ============================================================================

class CppMethod_CppPlain : public CppPlainBase
{
    Q_OBJECT
    QML_ELEMENT
public:
    using CppPlainBase::CppPlainBase;
    Q_INVOKABLE int value() const { return 2; }
};

class CppMethod_CppVirtual : public CppVirtualBase
{
    Q_OBJECT
    QML_ELEMENT
public:
    using CppVirtualBase::CppVirtualBase;
    Q_INVOKABLE int value() const { return 2; }
};

class CppMethod_CppFinal : public CppFinalBase
{
    Q_OBJECT
    QML_ELEMENT
public:
    using CppFinalBase::CppFinalBase;
    Q_INVOKABLE int value() const { return 2; }
};

class CppMethod_CppMethod : public CppMethodBase
{
    Q_OBJECT
    QML_ELEMENT
public:
    using CppMethodBase::CppMethodBase;
    Q_INVOKABLE int value() const { return 2; }
};

class CppMethod_CppOverride : public CppOverrideBase
{
    Q_OBJECT
    QML_ELEMENT
public:
    using CppOverrideBase::CppOverrideBase;
    Q_INVOKABLE int value() const { return 2; }
};

#endif // CPPTYPES_H
