// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// This example was created for the blog post about responsive layouts:
// https://www.qt.io/blog/responsive-layouts-in-qt

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

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
        Layout.fillWidth: false
        Layout.fillHeight: false
        Layout.alignment: Qt.AlignHCenter
        property string label: ""
        color: "#eaeaea"
        Text {
            font.pixelSize: 32
            font.bold: true
            anchors.centerIn: parent
            text: label
            color: "#243a5e"
        }
    }

    component ButtonBox : Rectangle {

        id: bb
        property Item elideButton: Item {}
        property Item buttonLayout: Item {}

        height: buttonLayout.height + buttonLayout.anchors.margins * 2
        implicitWidth: buttonLayout.width + buttonLayout.anchors.margins * 2

        color: "#c8c8c8"

        function updateItems() {

            if (width == 0)
                return;

            let butts = buttonLayout.children

            let lastBut = undefined
            let xmax = buttonLayout.anchors.margins
            for (let i = butts.length - 1; i>= 0; --i) {
                if (butts[i] !== elideButton && butts[i].visible) {
                    if (butts[i].x + butts[i].width > xmax) {
                        lastBut = butts[i]
                        xmax = butts[i].x + butts[i].width
                    }
                }
            }

            let buttonsRightX = xmax
                              + buttonLayout.spacing * 2
            let hiddenItems = 0
            for (let i = 0; i < butts.length; i++) {
                if (butts[i] === elideButton)
                    continue;
                if (butts[i].visible === false) {
                    hiddenItems++;
                    let buttonAddedWidth = butts[i].width + buttonLayout.spacing
                    if (buttonsRightX + buttonAddedWidth < width - elideButton.implicitWidth - buttonLayout.anchors.margins - buttonLayout.spacing) {
                        buttonsRightX += buttonAddedWidth
                        butts[i].visible = true
                        hiddenItems--;
                    }
                }
            }

            for (let i = butts.length - 1; i>= 0; --i) {
                if ( butts[i] === elideButton)
                    continue;
                if (butts[i].visible === true) {
                    let buttonRemovedWidth = butts[i].width + buttonLayout.spacing
                    if (buttonsRightX > width - elideButton.implicitWidth - buttonLayout.anchors.margins - buttonLayout.spacing) {
                        buttonsRightX -= buttonRemovedWidth
                        butts[i].visible = false
                        hiddenItems++;
                    }
                }
            }

            if (hiddenItems == 0)
                elideButton.visible = false
            else if (hiddenItems == 1) {
                for (let i = butts.length - 1; i>= 0; --i)
                    butts[i].visible = true
                elideButton.visible = false
            } else
                elideButton.visible = true
        }

        onWidthChanged: {
            updateItems()
        }

        onChildrenChanged: {
            updateItems()
        }
    }

    ColumnLayout {
        height: parent.height
        width: parent.width
        spacing: 0
        ButtonBox {
            Layout.fillHeight: false
            Layout.fillWidth: true

            RowLayout {
                id: buttonLayout
                anchors.margins: 10
                anchors.top: parent.top
                anchors.left: parent.left
                spacing: 8
                MyButton { id: aButton; label: "A"; z: 2 }
                MyButton { id: bButton; label: "B"; z: 2 }
                MyButton { id: cButton; label: "C"; z: 2 }
                MyButton { id: dButton; label: "D"; z: 2 }
                MyButton { id: eButton; label: "E"; z: 2 }
                MyButton { id: fButton; label: "F"; z: 2 }
                MyButton { id: gButton; label: "G"; z: 2 }
                MyButton { id: elideButton; label: "..."; z: 2 }
            }
            elideButton: elideButton
            buttonLayout: buttonLayout
        }

        Rectangle {
            id: rect
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#fff"
        }
    }
}

