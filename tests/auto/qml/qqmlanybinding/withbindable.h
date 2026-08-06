// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef WITH_BINDABLE_H
#define WITH_BINDABLE_H

#include <QObject>
#include <QtCore/qproperty.h>
#include <qqml.h>
#include <QtQml/private/qqmlproperty_p.h>
#include <QtQml/private/qqmlpropertyvalueinterceptor_p.h>

class WithBinding : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int prop READ prop WRITE setProp BINDABLE bindableProp)
public:
    int prop() {return m_prop;}
    void setProp(int i) {m_prop = i;}
    QBindable<int> bindableProp() {return &m_prop;}
private:
    QProperty<int> m_prop;
};

// Minimal stand-in for Behavior: redirects bindings to its own property.
class BindableInterceptor : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlPropertyValueInterceptor)

public:
    void setTarget(const QQmlProperty &property) override { m_property = property; }

    void write(const QVariant &value) override
    {
        ++writeCount;
        QQmlPropertyPrivate::write(m_property, value, QQmlPropertyData::BypassInterceptor);
    }

    bool bindable(QUntypedBindable *bindable, QUntypedBindable target) override
    {
        Q_UNUSED(target);
        ++bindableCount;
        *bindable = QBindable<int>(&m_intercepted);
        return true;
    }

    int writeCount = 0;
    int bindableCount = 0;

private:
    QQmlProperty m_property;
    QProperty<int> m_intercepted;
};

#endif

