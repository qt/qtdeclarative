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

#ifndef TESTATTACHEDTYPE_H
#define TESTATTACHEDTYPE_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <qqml.h>

class TestTypeAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int attachedCount READ getAttachedCount WRITE setAttachedCount BINDABLE
                       bindableAttachedCount NOTIFY attachedCountChanged)
    Q_PROPERTY(int attachedFormula READ getAttachedFormula WRITE setAttachedFormula BINDABLE
                       bindableAttachedFormula)
    Q_PROPERTY(QObject *attachedObject READ getAttachedObject WRITE setAttachedObject BINDABLE
                       bindableAttachedObject)
    QML_ANONYMOUS

    QProperty<int> m_count;
    QProperty<int> m_formula; // same as count but initialized with a script binding
    QProperty<QObject *> m_object;

public:
    TestTypeAttached(QObject *parent = nullptr);
    int getAttachedCount() const;
    void setAttachedCount(int v);
    QBindable<int> bindableAttachedCount();

    int getAttachedFormula() const;
    void setAttachedFormula(int v);
    QBindable<int> bindableAttachedFormula();

    QObject *getAttachedObject() const;
    void setAttachedObject(QObject *v);
    QBindable<QObject *> bindableAttachedObject();

signals:
    void triggered();

    void attachedCountChanged();
};

class TestType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(TestTypeAttached)

public:
    TestType(QObject *parent = nullptr);

    static TestTypeAttached *qmlAttachedProperties(QObject *);
};

#endif // TESTATTACHEDTYPE_H
