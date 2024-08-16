// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BASE_H
#define BASE_H

#include <QtCore/qobject.h>

class InaccessibleBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int a MEMBER m_a CONSTANT)
public:
    InaccessibleBase(QObject *parent = nullptr) : QObject(parent) {}
private:
    int m_a = 12;
};

#endif // BASE_H
