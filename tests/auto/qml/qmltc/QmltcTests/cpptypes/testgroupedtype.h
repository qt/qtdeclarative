// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

class QmlGeneralizedGroupPropertyTestType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("ImmediatePropertyNames", "myInt,group")

    Q_PROPERTY(TestTypeGrouped *group READ getGroup)

    TestTypeGrouped m_group;

public:
    QmlGeneralizedGroupPropertyTestType(QObject *parent = nullptr);

    TestTypeGrouped *getGroup();
};

class MyImmediateQtObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_CLASSINFO("ImmediatePropertyNames", "myInt");
};

#endif // TESTGROUPEDTYPE_H
