// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.MenuItem {
    id: control
    objectName: "menuitem-simple"

    implicitWidth: Math.max(contentItem.implicitWidth + indicator.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight, background.implicitHeight)

    arrow: Text {
        objectName: "menuitem-arrow-simple"
        text: control.mirrored ? "<" : ">"
    }

    indicator: Text {
        objectName: "menuitem-indicator-simple"
        text: control.checked ? "V" : ""
    }

    contentItem: Text {
        objectName: "menuitem-contentItem-simple"
        text: control.text
    }

    background: Rectangle {
        objectName: "menuitem-background-simple"
        implicitWidth: 20
        implicitHeight: 20
        color: control.pressed ? "red" : "green"
    }
}
