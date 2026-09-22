// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DERIVEDGADGET_H
#define DERIVEDGADGET_H

#include <QtQml/qqmlregistration.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

// A Q_GADGET/QML_VALUE_TYPE that derives from another one and adds properties
// and invokable methods of its own. AOT-compiled QML must not confuse the
// absolute property/method index (as returned by QMetaObject::indexOfProperty()
// or used to build QMetaObject::method()) with the index relative to the
// declaring class that qt_static_metacall() expects. See QTBUG-150640.
class BaseGadget
{
    Q_GADGET
    QML_VALUE_TYPE(baseGadget)
    Q_PROPERTY(int a READ a CONSTANT FINAL)
public:
    int a() const { return 1; }
    Q_INVOKABLE int baseMethod() const { return 42; }
};

class DerivedGadget : public BaseGadget
{
    Q_GADGET
    QML_VALUE_TYPE(derivedGadget)
    Q_PROPERTY(int b READ b CONSTANT FINAL)
    Q_PROPERTY(QString c READ c CONSTANT FINAL)
public:
    int b() const { return 2; }
    QString c() const { return QStringLiteral("three"); }
    Q_INVOKABLE int derivedMethod() const { return 99; }
};

class DerivedGadgetProvider : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DerivedGadget value READ value CONSTANT FINAL)
public:
    DerivedGadget value() const { return DerivedGadget(); }
};

#endif // DERIVEDGADGET_H
