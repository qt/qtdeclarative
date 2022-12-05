// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.SwitchDelegate {
    id: control
    objectName: "switchdelegate-simple"

    implicitWidth: Math.max(contentItem.implicitWidth + indicator.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight, background.implicitHeight)

    indicator: Text {
        objectName: "switchdelegate-indicator-simple"
        text: control.radioed ? "V" : ""
    }

    contentItem: Text {
        objectName: "switchdelegate-contentItem-simple"
        text: control.text
    }

    background: Rectangle {
        objectName: "switchdelegate-background-simple"
        implicitWidth: 20
        implicitHeight: 20
        color: control.pressed ? "red" : "green"
    }
}
