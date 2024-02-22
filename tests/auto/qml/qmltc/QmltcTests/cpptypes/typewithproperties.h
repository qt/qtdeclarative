// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TYPEWITHPROPERTIES_H
#define TYPEWITHPROPERTIES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmlregistration.h>
#include <QtQml/qjsvalue.h>

class TypeWithProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double a READ a WRITE setA BINDABLE bindableA)
    Q_PROPERTY(QString b READ b WRITE setB NOTIFY bChanged)
    Q_PROPERTY(QVariant c READ c WRITE setC NOTIFY cWeirdSignal)
    Q_PROPERTY(int d READ d WRITE setD NOTIFY dSignal BINDABLE bindableD)

    // special
    Q_PROPERTY(QJSValue jsvalue READ jsvalue WRITE setJsValue BINDABLE bindableJsValue)

    QProperty<double> m_a { 0.0 };
    QString m_b;
    QProperty<QVariant> m_c;
    QProperty<int> m_d;
    QProperty<QJSValue> m_jsvalue;

public:
    TypeWithProperties(QObject *parent = nullptr) : QObject(parent) { }

    double a() const;
    QString b() const;
    QVariant c() const;
    int d() const;

    QJSValue jsvalue() const;

    void setA(double);
    void setB(const QString &);
    void setC(const QVariant &);
    void setD(int);

    void setJsValue(const QJSValue &);

    QBindable<double> bindableA();
    QBindable<int> bindableD();
    QBindable<QJSValue> bindableJsValue();

Q_SIGNALS:
    void bChanged();
    // ### QTBUG-99317
    // void cWeirdSignal(const QVariant &);
    void cWeirdSignal(QVariant);
    void dSignal(QString, int);

    void signalWithEnum(Qt::MouseButtons buttons, Qt::MouseButton button);
};

#endif // TYPEWITHPROPERTIES_H
