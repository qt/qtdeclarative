// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

import MyStyle

T.ToolBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    background: Rectangle {
        implicitHeight: 40
        // Ensure that we use Control's attached MyStyle object by qualifying
        // the binding with its id. If we don't do this, an extra, unnecessary
        // attached MyStyle object will be created for the Rectangle.
        color: control.MyStyle.toolBarColor
    }
}
