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

//! [0]
    add: Transition {
        id: addTrans
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
        NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }

        PathAnimation {
            duration: 1000
            path: Path {
                startX: addTrans.ViewTransition.destination.x + 200
                startY: addTrans.ViewTransition.destination.y + 200
                PathCurve { relativeX: -100; relativeY: -50 }
                PathCurve { relativeX: 50; relativeY: -150 }
                PathCurve {
                    x: addTrans.ViewTransition.destination.x
                    y: addTrans.ViewTransition.destination.y
                }
            }
        }
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

    focus: true
    Keys.onSpacePressed: model.insert(0, { "name": "Item " + model.count })
}



