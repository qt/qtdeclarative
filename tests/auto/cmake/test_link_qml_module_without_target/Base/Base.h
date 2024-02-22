// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BASE_H
#define BASE_H

#include <QObject>

class Base : public QObject
{
    Q_OBJECT
public:
    explicit Base(QObject *parent = nullptr);
};

#endif // BASE_H
