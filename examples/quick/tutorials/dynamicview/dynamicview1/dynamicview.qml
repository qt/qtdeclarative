// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
//![0]
import QtQuick

Rectangle {
    id: root

    width: 300
    height: 400

//![1]
    Component {
        id: dragDelegate

        Rectangle {
            id: content

            required property string name
            required property string type
            required property string size
            required property int age

            width: view.width
            height: column.implicitHeight + 4

            border.width: 1
            border.color: "lightsteelblue"

            radius: 2

            Column {
                id: column
                anchors {
                    fill: parent
                    margins: 2
                }

                Text { text: qsTr('Name: ') + content.name }
                Text { text: qsTr('Type: ') + content.type }
                Text { text: qsTr('Age: ') + content.age }
                Text { text: qsTr('Size: ') + content.size }
            }
        }
    }
//![1]
//![2]
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
//![2]
}
//![0]
