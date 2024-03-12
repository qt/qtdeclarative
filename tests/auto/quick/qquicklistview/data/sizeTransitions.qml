// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtQuick.Window 2.1

Rectangle {
    id: root
    width: 500
    height: 600
    property int animationDuration: 10
    property int itemHeight: 40

    Rectangle {
        id: sightingsListPanel
        border.width: 2
        border.color: "lightgray"
        y: 200
        anchors.fill: parent
        anchors.topMargin: 200
        anchors.leftMargin: 200
        ListView {
            id: list
            objectName: "list"
            orientation: topToBottom ? ListView.Vertical : ListView.Horizontal
            property bool transitionFinished: false
            property bool scriptActionExecuted : false
            anchors { fill: parent; margins: parent.border.width; }
            model: testModel
            delegate: listDelegate
            // clip when we have no animation running
            clip: false
            add: Transition {
                id: trans
                onRunningChanged: {
                    if (!running)
                        list.transitionFinished = true;
                }
                SequentialAnimation {
                    ParallelAnimation {
                        NumberAnimation { properties: "x"; from: -100; duration: root.animationDuration }
                        NumberAnimation { properties: "y"; from: -100; duration: root.animationDuration }
                        NumberAnimation { properties: "width"; from: 1; to: list.width; duration: root.animationDuration;}
                        // Commenting out the height animation and it works
                        NumberAnimation { properties: "height"; from: 1; to: root.itemHeight; duration: root.animationDuration }
                    }
                    ScriptAction { script: list.scriptActionExecuted = true;}
                }

            }
        }
        // Delegate for defining a template for an item in the list
        Component {
            id: listDelegate
            Rectangle {
                id: background
                width: list.width
                height: root.itemHeight
                border.width: 2
                radius: 3
            }
        }
    }
}
