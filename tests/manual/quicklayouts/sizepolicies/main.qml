// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: appWindow

    visible: true
    title: qsTr("Basic layouts")
    readonly property int margin: 11

    Component.onCompleted: {
        width = mainLayout.implicitWidth + 2 * margin
        height = mainLayout.implicitHeight + 2 * margin
    }

    minimumWidth: mainLayout.Layout.minimumWidth + 2 * margin
    minimumHeight: mainLayout.Layout.minimumHeight + 2 * margin

    function colorToHex3(c) {
        return "#"
            + (Math.round(c.r*15).toString(16))
            + (Math.round(c.g*15).toString(16))
            + (Math.round(c.b*15).toString(16))
    }

    component ColorListView : ListView {
        id: lv
        model: 20
        clip: true
        spacing: 2
        delegate: Rectangle {
            id: root
            required property int index
            color: Qt.hsla(index / 19.0, 0.7, 0.9)
            width: lv.orientation === Qt.Horizontal ? 40 : lv.width
            height: lv.orientation === Qt.Horizontal ? lv.height : 20
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "" + colorToHex3(root.color)
            }
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: appWindow.margin
        RowLayout {
            Label { text: qsTr("Label & Slider") }
            Slider { }
        }
        RowLayout {
            TextField { placeholderText: qsTr("This wants to grow horizontally") }
            Button { text: qsTr("Button") }
        }
        RowLayout {
            TextField { placeholderText: qsTr("This wants to grow horizontally") }
            Slider { }
            Button { text: qsTr("Button") }
        }
        RowLayout {
            TextInput { text: qsTr("TextInput + TextField + Button") }
            TextField { placeholderText: qsTr("This wants to grow horizontally") }
            Button { text: qsTr("Button") }
        }
        RowLayout {
            ColorListView { orientation: Qt.Horizontal }
            ProgressBar { value: 0.5}
            Button { text: qsTr("Button") }
        }

        Label { text: qsTr("Line 1") }
        TextArea {
            text: qsTr("This widget spans over three rows in the GridLayout.\n")
                + qsTr("All items in the GridLayout are implicitly positioned from top to bottom.")
            wrapMode: TextArea.WordWrap
            Layout.minimumWidth: 100     // guesstimate, should be size of largest word
        }

        ColorListView {}
    }
}
