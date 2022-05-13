// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DUMMY_H
#define DUMMY_H

#include <QObject>

class Dummy : public QObject
{
    Q_OBJECT

public:
    Dummy(QObject *parent = nullptr);
    ~Dummy();
};

#endif // DUMMY_H

