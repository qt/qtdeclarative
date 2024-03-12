// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ToolButton {
    id: control
    objectName: "toolbutton-simple"

    implicitWidth: Math.max(contentItem.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, background.implicitHeight)

    contentItem: Text {
        objectName: "toolbutton-contentItem-simple"
        text: control.text
    }

    background: Rectangle {
        objectName: "toolbutton-background-simple"
        implicitWidth: 20
        implicitHeight: 20
        color: control.pressed ? "red" : "green"
    }
}
