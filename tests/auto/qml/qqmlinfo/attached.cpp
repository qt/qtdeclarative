// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "attached.h"

Attached::Attached(QObject *parent) :
    QObject(parent)
{
}

int Attached::a() const
{
    return mA;
}

void Attached::setA(int a)
{
    // Intentionally omit the early return in order to force a binding loop.

    mA = a;
    emit aChanged();
}

Attached *Attached::qmlAttachedProperties(QObject *object)
{
    return new Attached(object);
}
