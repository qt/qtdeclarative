// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.CheckDelegate {
    id: control
    objectName: "checkdelegate-simple"

    implicitWidth: Math.max(contentItem.implicitWidth + indicator.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight, background.implicitHeight)

    indicator: Text {
        objectName: "checkdelegate-indicator-simple"
        text: control.checked ? "V" : ""
    }

    contentItem: Text {
        objectName: "checkdelegate-contentItem-simple"
        text: control.text
    }

    background: Rectangle {
        objectName: "checkdelegate-background-simple"
        implicitWidth: 20
        implicitHeight: 20
        color: control.pressed ? "red" : "green"
    }
}
