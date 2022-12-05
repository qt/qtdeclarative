// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RangeSlider {
    id: control
    objectName: "rangeslider-simple"

    implicitWidth: Math.max(control.first.handle.implicitWidth, control.second.handle.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(control.first.handle.implicitHeight, control.second.handle.implicitHeight, background.implicitHeight)

    first.handle: Rectangle {
        objectName: "rangeslider-first.handle-simple"
        color: control.first.pressed ? "red" : "green"
    }

    second.handle: Rectangle {
        objectName: "rangeslider-second.handle-simple"
        color: control.second.pressed ? "red" : "green"
    }

    background: Rectangle {
        objectName: "rangeslider-background-simple"
        implicitWidth: 200
        implicitHeight: 20
    }
}
