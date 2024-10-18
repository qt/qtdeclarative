// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef INVISIBLE_H
#define INVISIBLE_H

#include <QtCore/qobject.h>

class InvisibleBase : public QObject
{
    Q_PROPERTY(int a MEMBER m_a CONSTANT)
public:
    InvisibleBase(QObject *parent = nullptr) : QObject(parent) {}
private:
    int m_a = 12;
};

#endif // BASE_H
