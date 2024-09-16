// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// This example was created for the blog post about responsive layouts:
// https://www.qt.io/blog/responsive-layouts-in-qt

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

Window {
    id: window

    width: 600
    height: 800

    minimumHeight: 500
    minimumWidth: 160

    title: "Window: (" + width + "x" + height + ")"
    visible: true


    component MyButton : Rectangle {
        implicitWidth: 56
        implicitHeight: 56
        Layout.minimumWidth: 56
        Layout.minimumHeight: 56
        Layout.fillWidth: true
        Layout.fillHeight: false
        Layout.alignment: Qt.AlignHCenter
        property string label: ""
        color: "#eaeaea"
        Rectangle {
            implicitWidth: 56
            implicitHeight: 56
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            color: "#eaeaea"
            Text {
                font.pixelSize: 32
                font.bold: true
                anchors.centerIn: parent
                text: label
                color: "#243a5e"
            }
        }
    }

    LayoutChooser {
        id: layoutChooser
        width: parent.width
        height: parent.height

        layoutChoices: [
            smallLayout,
            largeLayout
        ]

        criteria: [
            window.width < rl2.Layout.minimumWidth + rl2.anchors.margins * 2,
            true
        ]

        MyButton { id: aButton; label: "A"; z: 2 }
        MyButton { id: bButton; label: "B"; z: 2 }
        MyButton { id: cButton; label: "C"; z: 2 }
        MyButton { id: dButton; label: "D"; z: 2 }
        MyButton { id: eButton; label: "E"; z: 2 }
        MyButton { id: fButton; label: "F"; z: 2 }
        MyButton { id: gButton; label: "G"; z: 2 }

        Rectangle {
            id: rect
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#fff"
        }

        property Item smallLayout: RowLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width
            spacing: 0
            Rectangle {
                id: buttonRect1
                width: 200
                Layout.fillHeight: true
                color: "#c8c8c8"
                ColumnLayout {
                    anchors.margins: 10
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    spacing: 8
                    LayoutItemProxy { target: aButton }
                    LayoutItemProxy { target: bButton }
                    LayoutItemProxy { target: cButton }
                    LayoutItemProxy { target: dButton }
                    LayoutItemProxy { target: eButton }
                    LayoutItemProxy { target: fButton }
                    LayoutItemProxy { target: gButton }
                }
            }
            LayoutItemProxy { target: rect }
        }

        property Item largeLayout: ColumnLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width
            spacing: 0
            Rectangle {
                id: buttonRect2
                Layout.fillWidth: true
                height: rl2.height + rl2.anchors.margins * 2
                implicitWidth: rl2.width + rl2.anchors.margins * 2
                color: "#c8c8c8"
                RowLayout {
                    id: rl2
                    anchors.margins: 10
                    anchors.top: parent.top
                    anchors.left: parent.left
                    spacing: 8
                    LayoutItemProxy { target: aButton }
                    LayoutItemProxy { target: bButton }
                    LayoutItemProxy { target: cButton }
                    LayoutItemProxy { target: dButton }
                    LayoutItemProxy { target: eButton }
                    LayoutItemProxy { target: fButton }
                    LayoutItemProxy { target: gButton }
                }
            }
            LayoutItemProxy { target: rect }
        }
    }
}
