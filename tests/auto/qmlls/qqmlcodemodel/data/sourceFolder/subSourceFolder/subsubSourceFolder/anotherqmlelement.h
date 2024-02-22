// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ANOTHERQMLELEMENT_H
#define ANOTHERQMLELEMENT_H

#include <QObject>
#include <QQmlEngine>

class AnotherQmlElement : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit AnotherQmlElement(QObject *parent = nullptr);
};

#endif // ANOTHERQMLELEMENT_H
