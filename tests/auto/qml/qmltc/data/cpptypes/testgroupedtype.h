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

#ifndef TESTGROUPEDTYPE_H
#define TESTGROUPEDTYPE_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <qqml.h>

class TestTypeGrouped : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount NOTIFY countChanged)
    Q_PROPERTY(int formula READ getFormula WRITE setFormula BINDABLE bindableFormula)
    Q_PROPERTY(QObject *object READ getObject WRITE setObject BINDABLE bindableObject)
    Q_PROPERTY(QString str READ getStr WRITE setStr NOTIFY strChanged);
    QML_ANONYMOUS

    QProperty<int> m_count;
    QProperty<int> m_formula; // same as count but initialized with a script binding
    QProperty<QObject *> m_object;
    QString m_str;

public:
    TestTypeGrouped(QObject *parent = nullptr);
    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();

    int getFormula() const;
    void setFormula(int v);
    QBindable<int> bindableFormula();

    QObject *getObject() const;
    void setObject(QObject *v);
    QBindable<QObject *> bindableObject();

    QString getStr() const;
    void setStr(const QString &s);

signals:
    void triggered();
    void countChanged();
    void strChanged();
};

// Note: unlike attached property, this type is the parent of the QML type
class QmlGroupPropertyTestType : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(TestTypeGrouped *group READ getGroup)

    TestTypeGrouped m_group;

public:
    QmlGroupPropertyTestType(QObject *parent = nullptr);

    TestTypeGrouped *getGroup();
};

#endif // TESTGROUPEDTYPE_H
