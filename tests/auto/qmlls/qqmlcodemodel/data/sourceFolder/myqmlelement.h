// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYQMLELEMENT_H
#define MYQMLELEMENT_H

#include <QObject>
#include <QQmlEngine>

class MyQmlElement : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit MyQmlElement(QObject *parent = nullptr);

signals:

};

#endif // MYQMLELEMENT_H
