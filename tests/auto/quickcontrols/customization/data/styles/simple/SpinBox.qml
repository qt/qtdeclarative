// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.SpinBox {
    id: control
    objectName: "spinbox-simple"

    implicitWidth: Math.max(contentItem.implicitWidth + up.indicator.implicitWidth + down.indicator.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, up.indicator.implicitHeight, down.indicator.implicitHeight, background.implicitHeight)

    up.indicator: Rectangle {
        objectName: "spinbox-up.indicator-simple"
        color: control.up.pressed ? "red" : "green"
    }

    down.indicator: Rectangle {
        objectName: "spinbox-down.indicator-simple"
        color: control.down.pressed ? "red" : "green"
    }

    contentItem: Text {
        objectName: "spinbox-contentItem-simple"
    }

    background: Rectangle {
        objectName: "spinbox-background-simple"
        implicitWidth: 200
        implicitHeight: 20
    }
}
