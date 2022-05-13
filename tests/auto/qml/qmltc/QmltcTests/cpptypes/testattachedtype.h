// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    static int creationCount;
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
