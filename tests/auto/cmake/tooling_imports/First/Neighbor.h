// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <QtQml/qqml.h>

class Neighbor : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Neighbor)

public:
    Neighbor(QObject *parent = nullptr);
};

class Absent : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNAVAILABLE
};

#endif // NEIGHBOR_H
