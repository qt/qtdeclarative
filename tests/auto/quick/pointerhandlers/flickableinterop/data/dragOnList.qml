// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        objectName: "itemview delegate"
        color: delegateDrag.active ? "wheat" : "beige"
        border.color: "black"
        width: parent.width; height: 140
        Rectangle {
            objectName: "button"
            x: 350; y: 20
            border.color: "tomato"
            border.width: 10
            color: buttonDrag.active ? "goldenrod" : "beige"
            width: 100
            height: 100
            DragHandler {
                id: buttonDrag
                objectName: "buttonDrag"
            }
            Component.onCompleted: if (!root.buttonUnderTest && index == 2) {
                root.buttonUnderTest = this
                root.delegateUnderTest = parent
            }
        }
        DragHandler {
            id: delegateDrag
            objectName: "delegateDrag " + index
        }
    }

    DragHandler {
        objectName: "contentItemDrag"
    }

    Component.onCompleted: contentItem.objectName = "ListView's contentItem"
}
