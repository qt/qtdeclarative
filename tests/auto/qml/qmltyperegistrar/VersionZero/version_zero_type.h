// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VERSION_ZERO_TYPE_H
#define VERSION_ZERO_TYPE_H

#include <QtQml/qqml.h>

class TypeInModuleMajorVersionZero : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    TypeInModuleMajorVersionZero(QObject *parent = nullptr) : QObject(parent) {}
};

#endif // VERSION_ZERO_TYPE_H
