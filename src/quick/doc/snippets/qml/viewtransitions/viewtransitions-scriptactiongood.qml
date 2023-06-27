// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

ListView {
    width: 240; height: 320
    model: ListModel {
        Component.onCompleted: {
            for (var i=0; i<8; i++)
                append({"name": i})
        }
    }

    delegate: Rectangle {
        width: 100; height: 30
        border.width: 1
        color: "lightsteelblue"
        Text {
            anchors.centerIn: parent
            text: name
        }
        objectName: name
    }

//! [0]
    move: Transition {
        id: moveTrans
        SequentialAnimation {
            ColorAnimation { property: "color"; to: "yellow"; duration: 400 }
            NumberAnimation { properties: "x,y"; duration: 800; easing.type: Easing.OutBack }
            //ScriptAction { script: moveTrans.ViewTransition.item.color = "lightsteelblue" } BAD!

            PropertyAction { property: "color"; value: "lightsteelblue" }
        }
    }
//! [0]

    displaced: Transition {
        NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutBounce }
    }

    focus: true
    Keys.onSpacePressed: model.move(5, 1, 3)
}


