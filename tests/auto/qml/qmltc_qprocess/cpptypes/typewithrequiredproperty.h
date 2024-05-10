// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TYPEWITHREQUIREDPROPERTY_H_
#define TYPEWITHREQUIREDPROPERTY_H_

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>

class TypeWithRequiredProperty : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString requiredProperty READ requiredProperty WRITE setRequiredProperty REQUIRED)

    QProperty<QString> m_requiredProperty;

public:
    TypeWithRequiredProperty(QObject *parent = nullptr) : QObject(parent) { }

    QString requiredProperty() const { return m_requiredProperty; }
    void setRequiredProperty(const QString &s) { m_requiredProperty = s; }
};

#endif // TYPEWITHREQUIREDPROPERTY_H_
