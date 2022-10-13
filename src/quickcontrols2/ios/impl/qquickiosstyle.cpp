// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickiosstyle_p.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QString, GlobalPath, (QLatin1String("qrc:/qt-project.org/imports/QtQuick/Controls/iOS/images/")))

QQuickIOSStyle::QQuickIOSStyle(QObject *parent)
    : QObject(parent)
{
}

QUrl QQuickIOSStyle::url()
{
    return *GlobalPath();
}

QT_END_NAMESPACE
