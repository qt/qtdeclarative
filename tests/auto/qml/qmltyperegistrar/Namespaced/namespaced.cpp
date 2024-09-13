// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "namespaced.h"

#include <QtCore/qlatin1stringview.h>

// Have a symbol that can be missing if not linked properly.
Namespaced::Namespaced(QObject *parent) : QObject(parent)
{
    setObjectName(QLatin1String("namespaced"));
}
