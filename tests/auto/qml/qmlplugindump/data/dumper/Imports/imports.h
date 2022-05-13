// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef IMPORTS_H
#define IMPORTS_H

#include <QObject>

class Imports : public QObject
{
    Q_OBJECT

public:
    Imports(QObject *parent = nullptr);
    ~Imports();
};

#endif // IMPORTS_H

