// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYQMLELEMENT2_H
#define MYQMLELEMENT2_H

#include <QObject>
#include <QQmlEngine>

class myQmlElement2 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit myQmlElement2(QObject *parent = nullptr);
};

#endif // MYQMLELEMENT2_H
