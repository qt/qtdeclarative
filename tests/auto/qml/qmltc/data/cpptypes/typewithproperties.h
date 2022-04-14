/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
};

#endif // TYPEWITHPROPERTIES_H
