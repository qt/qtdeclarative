// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmacfocusframe.h"

#include <AppKit/AppKit.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <QtQml/qqmlcomponent.h>

QT_BEGIN_NAMESPACE

QQuickItem *QQuickMacFocusFrame::createFocusFrame(QQmlContext *context)
{
    QQmlComponent component(
            context->engine(),
            QUrl(QStringLiteral(
                    "qrc:/qt-project.org/imports/QtQuick/NativeStyle/util/MacFocusFrame.qml")));
    auto frame = qobject_cast<QQuickItem *>(component.create());
    if (!frame)
        return nullptr;

    auto indicatorColor = qt_mac_toQColor(NSColor.keyboardFocusIndicatorColor.CGColor);
    indicatorColor.setAlpha(255);
    frame->setProperty("systemFrameColor", indicatorColor);
    return frame;
}

QT_END_NAMESPACE
