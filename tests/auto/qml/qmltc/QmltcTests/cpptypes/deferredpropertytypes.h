// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DEFERREDPROPERTYTYPES_H
#define DEFERREDPROPERTYTYPES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/qquickitem.h>

#include "testgroupedtype.h"
#include "testattachedtype.h"

// normal properties:

class TypeWithDeferredProperty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DeferredPropertyNames", "deferredProperty")

    Q_PROPERTY(QQuickItem *deferredProperty READ deferredProperty WRITE setDeferredProperty BINDABLE
                       bindableDeferredProperty)

    QProperty<QQuickItem *> m_deferredProperty { nullptr };

public:
    TypeWithDeferredProperty(QObject *parent = nullptr) : QObject(parent) { }

    QQuickItem *deferredProperty() const;
    void setDeferredProperty(QQuickItem *);
    QBindable<QQuickItem *> bindableDeferredProperty();
};

// group properties:

class TestTypeGroupedWithDeferred : public TestTypeGrouped
{
    Q_OBJECT
    Q_PROPERTY(int deferred READ getDeferred WRITE setDeferred BINDABLE bindableDeferred)
    QML_ANONYMOUS
    Q_CLASSINFO("DeferredPropertyNames", "deferred")

    QProperty<int> m_deferred { 0 };

public:
    int getDeferred() const { return m_deferred; }
    void setDeferred(int v) { m_deferred = v; }
    QBindable<int> bindableDeferred() const { return QBindable<int>(&m_deferred); }
};

class TypeWithDeferredGroup : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(TestTypeGroupedWithDeferred *group READ getGroup)

    TestTypeGroupedWithDeferred m_group;

public:
    TypeWithDeferredGroup(QObject *parent = nullptr) : QObject(parent) { }

    TestTypeGroupedWithDeferred *getGroup() { return &m_group; }
};

// attached properties:

class TestTypeAttachedWithDeferred : public TestTypeAttached
{
    Q_OBJECT
    Q_PROPERTY(int deferred READ getDeferred WRITE setDeferred BINDABLE bindableDeferred)
    QML_ANONYMOUS
    Q_CLASSINFO("DeferredPropertyNames", "deferred")

    QProperty<int> m_deferred { 0 };

public:
    TestTypeAttachedWithDeferred(QObject *parent) : TestTypeAttached(parent) { }

    int getDeferred() const { return m_deferred; }
    void setDeferred(int v) { m_deferred = v; }
    QBindable<int> bindableDeferred() const { return QBindable<int>(&m_deferred); }
};

class DeferredAttached : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(TestTypeAttachedWithDeferred)

public:
    DeferredAttached(QObject *parent = nullptr) : QObject(parent) { }

    static TestTypeAttachedWithDeferred *qmlAttachedProperties(QObject *);
};

// special:

class TypeWithDeferredComplexProperties : public TypeWithDeferredGroup
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DeferredPropertyNames", "group,DeferredAttached")

public:
    TypeWithDeferredComplexProperties(QObject *parent = nullptr) : TypeWithDeferredGroup(parent) { }
};

#endif // DEFERREDPROPERTYTYPES_H
