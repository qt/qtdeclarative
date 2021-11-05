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
#ifndef WithBindableProperties_H
#define WithBindableProperties_H

#include <QObject>
#include <qqml.h>
#include <QtCore/qproperty.h>


class WithBindableProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int a READ a WRITE setA BINDABLE bindableA)
    Q_PROPERTY(int b READ b WRITE setB BINDABLE bindableB)

public:
    QProperty<int> m_a;
    QProperty<int> m_b;
    int a() {return m_a;}
    int b() {return m_b;}
    void setA(int val) {m_a = val;}
    void setB(int val) {m_b = val;}
    QBindable<int> bindableA() {return QBindable<int>(&m_a); }
    QBindable<int> bindableB() {return QBindable<int>(&m_b); }
};

#endif

