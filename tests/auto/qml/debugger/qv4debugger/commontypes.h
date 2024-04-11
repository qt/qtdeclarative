// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <QtQuick/qquickitem.h>
#include <QtQml/qqmlregistration.h>
#include <QtQml/private/qv4engine_p.h>

class MyType : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    MyType(QQuickItem *parent = nullptr) : QQuickItem(parent) {}
    Q_INVOKABLE void name(QQmlV4FunctionPtr) const {}
};

#endif // COMMONTYPES_H
