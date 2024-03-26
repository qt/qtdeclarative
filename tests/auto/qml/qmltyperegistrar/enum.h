// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ENUM_NS_HELLO_H
#define ENUM_NS_HELLO_H

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace Hello {
    Q_NAMESPACE
    QML_NAMED_ELEMENT(World)
    enum class World {
        Europe = 2024,
    };
    Q_ENUM_NS(World)
}

#endif // ENUM_NS_HELLO_H
