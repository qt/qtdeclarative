// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

Rectangle {
    id: root

    width: 300
    height: 400

//![0]
    Component {
        id: dragDelegate

//![1]
        MouseArea {
            id: dragArea

            property bool held: false
            required property string name
            required property string type
            required property string size
            required property int age

            anchors {
                left: parent?.left
                right: parent?.right
            }
            height: content.height

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressAndHold: held = true
            onReleased: held = false

            Rectangle {
                id: content
//![1]
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: dragArea.width
                height: column.implicitHeight + 4

                border.width: 1
                border.color: "lightsteelblue"
//![3]
                color: dragArea.held ? "lightsteelblue" : "white"
                Behavior on color { ColorAnimation { duration: 100 } }
//![3]
                radius: 2
//![4]
                states: State {
                    when: dragArea.held

                    ParentChange {
                        target: content
                        parent: root
                    }
                    AnchorChanges {
                        target: content
                        anchors {
                            horizontalCenter: undefined
                            verticalCenter: undefined
                        }
                    }
                }
//![4]
                Column {
                    id: column
                    anchors {
                        fill: parent
                        margins: 2
                    }

                    Text { text: qsTr('Name: ') + dragArea.name }
                    Text { text: qsTr('Type: ') + dragArea.type }
                    Text { text: qsTr('Age: ') + dragArea.age }
                    Text { text: qsTr('Size: ') + dragArea.size }
                }
//![2]
            }
        }
//![2]
    }
//![0]

    ListView {
        id: view

        anchors {
            fill: parent
            margins: 2
        }

        model: PetsModel {}
        delegate: dragDelegate

        spacing: 4
        cacheBuffer: 50
    }
}
