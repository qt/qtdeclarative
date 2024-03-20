// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CUSTOMINITIALIAZATION_H_
#define CUSTOMINITIALIAZATION_H_

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQuick/qquickitem.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlregistration.h>

class ExtensionType : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(double propertyFromExtension READ getPropertyFromExtension WRITE
                       setPropertyFromExtension)
    Q_PROPERTY(QQmlListProperty<QQuickItem> extensionObjectList READ getExtensionObjectList)

    QProperty<double> m_propertyFromExtension{ 0 };
    QList<QQuickItem *> m_extensionObjectList;

public:
    ExtensionType(QObject *parent = nullptr) : QObject(parent) { }

    double getPropertyFromExtension() const { return m_propertyFromExtension; }
    void setPropertyFromExtension(double v) { m_propertyFromExtension = v; }

    QQmlListProperty<QQuickItem> getExtensionObjectList()
    {
        return QQmlListProperty<QQuickItem>(this, &m_extensionObjectList);
    }
};

class TypeForCustomInitialization : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQmlListProperty<QQuickItem> cppObjectList READ getCppObjectList)
    Q_PROPERTY(
            double defaultedBindable BINDABLE bindableDefaultedBindable READ default WRITE default)

    QML_EXTENDED(ExtensionType)

    QList<QQuickItem *> m_cppObjectList;
    QProperty<double> m_defaultedBindable;

public:
    TypeForCustomInitialization(QObject *parent = nullptr) : QObject(parent) { }

    QQmlListProperty<QQuickItem> getCppObjectList()
    {
        return QQmlListProperty<QQuickItem>(this, &m_cppObjectList);
    }

    QBindable<double> bindableDefaultedBindable()
    {
        return QBindable<double>(&m_defaultedBindable);
    }
};

#endif // CUSTOMINITIALIAZATION_H_
