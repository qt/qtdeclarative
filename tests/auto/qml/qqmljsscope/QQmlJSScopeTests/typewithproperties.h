// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TYPEWITHPROPERTIES_H
#define TYPEWITHPROPERTIES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmlregistration.h>

class TypeWithProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double a READ a WRITE setA NOTIFY aChanged)

    QProperty<double> m_a { 0.0 };

public:
    TypeWithProperties(QObject *parent = nullptr) : QObject(parent) { }
    double a() const { return m_a; }
    void setA(double a_) { m_a = a_; }

Q_SIGNALS:
    void aChanged();
};

#endif // TYPEWITHPROPERTIES_H
