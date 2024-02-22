// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.11
import QtQuick.Window 2.11

Window {
    id: root
    property alias timer : timer
    property variant ops: [{'op': 'add', 'count': 3}, {'op': 'add', 'count': 6}, {'op': 'rem', 'count': 4}, {'op': 'rem', 'count': 1}, {'op': 'rem', 'count': 3}]
    property int opIndex : 0
    width: 400
    height: 600

    ListModel {
        id: theModel
    }

    Timer {
        id: timer
        interval: 100
        running: false
        repeat: true
        onTriggered: {
            if (opIndex >= ops.length) {
                timer.stop()
                return
            }
            let op = ops[opIndex]
            for (var i = 0; i < op.count; ++i) {
                if (op.op === "add")
                    theModel.append({"name": "opIndex " + opIndex})
                else
                    theModel.remove(0, 1);
            }
            opIndex = opIndex + 1
        }
    }

    ListView {
        anchors.top: parent.top
        anchors.right: parent.right
        height: 600
        anchors.left: parent.horizontalCenter
        spacing: 4
        model: theModel
        header: Text {
            text: "YAnimator"
        }
        add: Transition {
            NumberAnimation { property: "scale";   from: 0; to: 1; duration: 200 }
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
        }
        displaced: Transition {
            YAnimator { duration: 500 }
            NumberAnimation { property: "opacity"; to: 1.0; duration: 500 }
            NumberAnimation { property: "scale";   to: 1.0; duration: 500 }
        }
        remove: Transition {
            NumberAnimation { property: "opacity"; to: 0; duration: 200 }
            NumberAnimation { property: "scale";   to: 0; duration: 200 }
        }
        delegate: Rectangle {
            width: 200
            height: 20
            color:"red"
            Text {
                anchors.centerIn: parent
                text: name
            }
        }
    }
}
