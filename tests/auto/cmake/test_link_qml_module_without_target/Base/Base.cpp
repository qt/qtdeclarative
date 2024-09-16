// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "Base.h"
#include <QDebug>

Base::Base(QObject *parent) : QObject(parent)
{
    qDebug() << Q_FUNC_INFO << "Base library loaded";
}
