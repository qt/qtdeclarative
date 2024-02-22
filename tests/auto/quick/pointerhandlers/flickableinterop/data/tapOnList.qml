// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

ListView {
    id: root
    objectName: "root"
    property Item buttonUnderTest: null
    property Item delegateUnderTest: null
    width: 800
    height: 480
    model: 10
    spacing: 2

    // faster rebound to speed up test runs
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }

    delegate: Rectangle {
        objectName: "itemview delegate " + index
        color: delegateTap.pressed ? "wheat" : "beige"
        width: parent.width; height: 140
        Text { text: index }
        Rectangle {
            objectName: "button " + index
            anchors.centerIn: parent
            border.color: "tomato"
            border.width: 10
            color: innerTap.pressed ? "goldenrod" : "beige"
            width: 100
            height: 100
            TapHandler {
                id: innerTap
                objectName: "buttonTap " + index
            }
            Component.onCompleted: if (!root.buttonUnderTest && index == 2) {
                root.buttonUnderTest = this
                root.delegateUnderTest = parent
            }
        }
        TapHandler {
            id: delegateTap
            objectName: "delegateTap " + index
        }
    }

    TapHandler {
        objectName: "contentItemTap"
    }

    Component.onCompleted: contentItem.objectName = "ListView's contentItem"
}
