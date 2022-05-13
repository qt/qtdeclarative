// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.11
import QtQuick.Window 2.11

Window {
    id: root
    property alias timer : timer
    property alias listView : listView
    property alias theModel: theModel
    property variant ops: [{'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39},
    {'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39},
    {'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39},
    {'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39},
    {'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39},
    {'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39},
    {'op': 'add', 'count': 30}, {'op': 'add', 'count': 60}, {'op': 'rem', 'count': 40}, {'op': 'rem', 'count': 10}, {'op': 'rem', 'count': 39}]
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
        id: listView
        anchors.fill: parent
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
            NumberAnimation { property: "opacity"; to: 1.0; duration: 1000 }
            NumberAnimation { property: "scale";   to: 1.0; duration: 1000 }
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
