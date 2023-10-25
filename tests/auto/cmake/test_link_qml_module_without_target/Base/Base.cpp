// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include "Base.h"
#include <QDebug>

Base::Base(QObject *parent) : QObject(parent)
{
    qDebug() << Q_FUNC_INFO << "Base library loaded";
}
