// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TYPEWITHMEMBERPROPERTIES_H
#define TYPEWITHMEMBERPROPERTIES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>

class TypeWithMemberProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int x MEMBER m_x)
    Q_PROPERTY(QString y MEMBER m_y)

    QProperty<int> m_x;

public:
    QProperty<QString> m_y;

    TypeWithMemberProperties(QObject *parent = nullptr) : QObject(parent) { }
};

#endif // TYPEWITHMEMBERPROPERTIES_H
