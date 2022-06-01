/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
