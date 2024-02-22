// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.MenuBar {
    id: control
    objectName: "menubar-simple"

    implicitWidth: Math.max(contentItem.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, background.implicitHeight)

    contentItem: Item {
        objectName: "menubar-contentItem-simple"
    }

    background: Rectangle {
        objectName: "menubar-background-simple"
        implicitWidth: 20
        implicitHeight: 20
        color: control.pressed ? "red" : "green"
    }
}
