// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    property string text

    function show() {
        showAnimation.restart();
    }

    height: 42
    width: warningIcon.width + textItem.width + 40
    opacity: 0

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.6
    }

    SequentialAnimation {
        id: showAnimation
        NumberAnimation {
            target: rootItem
            property: "opacity"
            to: 1
            duration: 400
            easing.type: Easing.InOutQuad
        }
        PauseAnimation {
            duration: 2000
        }
        NumberAnimation {
            target: rootItem
            property: "opacity"
            to: 0
            duration: 400
            easing.type: Easing.InOutQuad
        }
    }

    Image {
        id: warningIcon
        anchors.verticalCenter: parent.verticalCenter
        x: 8
        source: "images/warning.png"
        mipmap: true
        width: 24
        height: width
    }

    Text {
        id: textItem
        anchors.left: warningIcon.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        color: "#ffffff"
        font.pixelSize: 16
        text: rootItem.text
    }

}
