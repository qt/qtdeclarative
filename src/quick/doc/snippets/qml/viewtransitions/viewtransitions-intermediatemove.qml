// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

ListView {
    width: 240; height: 320
    model: ListModel {}

    delegate: Rectangle {
        width: 100; height: 30
        border.width: 1
        color: "lightsteelblue"
        Text {
            anchors.centerIn: parent
            text: name
        }
    }

    add: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
        NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
    }

//! [0]
    displaced: Transition {
        id: dispTrans
        SequentialAnimation {
            PauseAnimation {
                duration: (dispTrans.ViewTransition.index -
                        dispTrans.ViewTransition.targetIndexes[0]) * 100
            }
            ParallelAnimation {
                NumberAnimation {
                    property: "x"; to: dispTrans.ViewTransition.item.x + 20
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    property: "y"; to: dispTrans.ViewTransition.item.y + 50
                    easing.type: Easing.OutQuad
                }
            }
            NumberAnimation { properties: "x,y"; duration: 500; easing.type: Easing.OutBounce }
        }
    }

//! [0]

    focus: true
    Keys.onSpacePressed: model.insert(0, { "name": "Item " + model.count })
}


