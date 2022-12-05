// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Slider {
    id: control
    objectName: "slider-simple"

    implicitWidth: Math.max(handle.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(handle.implicitHeight, background.implicitHeight)

    handle: Rectangle {
        objectName: "slider-handle-simple"
        color: control.pressed ? "red" : "green"
    }

    background: Rectangle {
        objectName: "slider-background-simple"
        implicitWidth: 200
        implicitHeight: 20
    }
}
