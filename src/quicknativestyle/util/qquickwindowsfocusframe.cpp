// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindowsfocusframe.h"

#include <QtQml/qqmlcomponent.h>

QT_BEGIN_NAMESPACE

QQuickItem *QQuickWindowsFocusFrame::createFocusFrame(QQmlContext *context)
{
    QQmlComponent component(context->engine(),
        QUrl(QStringLiteral("qrc:/qt-project.org/imports/QtQuick/NativeStyle/util/WindowsFocusFrame.qml")));
    return qobject_cast<QQuickItem *>(component.create());
}

QT_END_NAMESPACE
