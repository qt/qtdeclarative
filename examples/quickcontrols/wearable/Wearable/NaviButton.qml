// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle

QQC2.AbstractButton {
    id: button

    property int edge: Qt.TopEdge
    property alias imageSource: image.source

    contentItem: Image {
        id: image
        fillMode: Image.Pad
        sourceSize { width: 40; height: 40 } // ### TODO: resize the image
    }

    background: Rectangle {
        height: button.height * 4
        width: height
        radius: width / 2

        anchors.horizontalCenter: button.horizontalCenter
        anchors.top: edge === Qt.BottomEdge ? button.top : undefined
        anchors.bottom: edge === Qt.TopEdge ? button.bottom : undefined

        color: UIStyle.colorQtGray2
    }

    transform: Translate {
        Behavior on y { NumberAnimation { } }
        y: enabled ? 0 : edge === Qt.TopEdge ? -button.height : button.height
    }
}
