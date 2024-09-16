// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

TableView {
    id: root
    objectName: "root"
    property Item buttonUnderTest: null
    property Item delegateUnderTest: null
    width: 800
    height: 480
    columnSpacing: 2
    rowSpacing: 2
    // TODO use TableModel when it's ready, to test with multiple columns
    model: 10

    // faster rebound to speed up test runs
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }

    delegate: Rectangle {
        id: tableDelegate
        objectName: "itemview delegate"
        color: delegateTap.pressed ? "wheat" : "beige"
        implicitWidth: 200
        implicitHeight: 140

        Rectangle {
            objectName: "button"
            anchors.centerIn: parent
            border.color: "tomato"
            border.width: 10
            color: innerTap.pressed ? "goldenrod" : "beige"
            width: 100
            height: 100
            TapHandler {
                id: innerTap
                objectName: "buttonTap"
            }
            Component.onCompleted: if (!root.buttonUnderTest && index == 2) {
                root.buttonUnderTest = this
                root.delegateUnderTest = parent
            }
        }

        TapHandler {
            id: delegateTap
            objectName: "delegateTap"
        }
    }

    TapHandler {
        objectName: "contentItemTap"
    }

    Component.onCompleted: contentItem.objectName = "TableView's contentItem"
}
