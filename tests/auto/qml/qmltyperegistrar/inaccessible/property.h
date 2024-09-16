// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PROPERTY_H
#define PROPERTY_H

#include <QtCore/qobject.h>

class InaccessibleProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int b MEMBER m_b CONSTANT)
public:
    InaccessibleProperty(QObject *parent = nullptr) : QObject(parent) {}
private:
    int m_b = 13;
};

#endif // PROPERTY_H
