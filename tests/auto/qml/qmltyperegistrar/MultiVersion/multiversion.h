// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MULTIVERSION_H
#define MULTIVERSION_H

#include <QtQml/qqml.h>

class MultiVersion : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Multi)
    QML_NAMED_ELEMENT(Version)
    QML_ADDED_IN_VERSION(1, 4)
    QML_EXTRA_VERSION(2, 0)
public:
    MultiVersion(QObject *parent = nullptr) : QObject(parent) {}
};

#endif // MULTIVERSION_H
