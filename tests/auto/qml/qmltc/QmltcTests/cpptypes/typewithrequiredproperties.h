// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQuick/qquickitem.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlregistration.h>

#ifndef TYPEWITHREQUIREDPROPERTIES_H_
#  define TYPEWITHREQUIREDPROPERTIES_H_

class ExtensionTypeWithRequiredProperties : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(double requiredPropertyFromExtension READ getRequiredPropertyFromExtension WRITE
                       setRequiredPropertyFromExtension REQUIRED)

    QProperty<double> m_requiredPropertyFromExtension{};

public:
    ExtensionTypeWithRequiredProperties(QObject *parent = nullptr) : QObject(parent) { }

    double getRequiredPropertyFromExtension() const { return m_requiredPropertyFromExtension; }
    void setRequiredPropertyFromExtension(double v) { m_requiredPropertyFromExtension = v; }
};

class TypeWithRequiredProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem *inheritedRequiredProperty READ getInheritedRequiredProperty WRITE
                       setInheritedRequiredProperty REQUIRED)
    Q_PROPERTY(int inheritedRequiredPropertyThatWillBeBound READ
                       getInheritedRequiredPropertyThatWillBeBound WRITE
                               setInheritedRequiredPropertyThatWillBeBound REQUIRED)
    Q_PROPERTY(int nonRequiredInheritedPropertyThatWillBeMarkedRequired READ
                       getNonRequiredInheritedPropertyThatWillBeMarkedRequired WRITE
                               setNonRequiredInheritedPropertyThatWillBeMarkedRequired REQUIRED)

    QML_EXTENDED(ExtensionTypeWithRequiredProperties)

    QProperty<QQuickItem *> m_inheritedRequiredProperty{};
    QProperty<int> m_inheritedRequiredPropertyThatWillBeBound{};
    QProperty<int> m_nonRequiredInheritedPropertyThatWillBeMarkedRequired{};

public:
    TypeWithRequiredProperties(QObject *parent = nullptr) : QObject(parent) { }

    QQuickItem *getInheritedRequiredProperty() const { return m_inheritedRequiredProperty; }
    void setInheritedRequiredProperty(QQuickItem *v) { m_inheritedRequiredProperty = v; }

    int getInheritedRequiredPropertyThatWillBeBound() const
    {
        return m_inheritedRequiredPropertyThatWillBeBound;
    }
    void setInheritedRequiredPropertyThatWillBeBound(int v)
    {
        m_inheritedRequiredPropertyThatWillBeBound = v;
    }

    int getNonRequiredInheritedPropertyThatWillBeMarkedRequired() const
    {
        return m_nonRequiredInheritedPropertyThatWillBeMarkedRequired;
    }
    void setNonRequiredInheritedPropertyThatWillBeMarkedRequired(int v)
    {
        m_nonRequiredInheritedPropertyThatWillBeMarkedRequired = v;
    }
};

#endif // TYPEWITHREQUIREDPROPERTIES_H_
