// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.4
import QtQuick.Window 2.2
import "qrc:/quick/shared/" as Examples

Window {
    width: 800
    height: 600
    visible: true
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("crosshairs", "crosshairs and velocity vectors at finger positions", Qt.resolvedUrl("mpta-crosshairs.qml"))
            addExample("flickable", "flick a bigger Rectangle", Qt.resolvedUrl("basic-flickable.qml"))
            addExample("flick and resize", "flick and resize an image (old-school PinchArea)", Qt.resolvedUrl("flickresize.qml"))
            addExample("flick text", "flick a text document", Qt.resolvedUrl("flicktext.qml"))
        }
    }
}
