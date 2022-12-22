// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typewithnamespace.h"

MyNamespace::TypeWithNamespace::TypeWithNamespace(QObject *parent) : QObject{ parent } { }

MyNamespace::Sub1::Sub2::Sub3::TypeWithSubnamespace::TypeWithSubnamespace(QObject *parent)
    : QObject{ parent }
{
}
