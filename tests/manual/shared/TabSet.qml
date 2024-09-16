// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Item {
    id: tabWidget

    // Setting the default property to stack.children means any child items
    // of the TabWidget are actually added to the 'stack' item's children.
    // See the "Property Binding"
    // documentation for details on default properties.
    default property alias content: stack.children

    property int current: 0

    onCurrentChanged: setZOrders()
    Component.onCompleted: setZOrders()

    function setZOrders() {
        for (var i = 0; i < stack.children.length; ++i) {
            stack.children[i].z = (i == current ? 1 : 0)
            stack.children[i].enabled = (i == current)
        }
    }

    Row {
        id: header

        Repeater {
            model: stack.children.length
            delegate: Rectangle {
                required property int index
                width: tabWidget.width / stack.children.length
                height: Math.max(Screen.pixelDensity * 7, label.implicitHeight * 1.2)

                Rectangle {
                    width: parent.width; height: 1
                    anchors { bottom: parent.bottom; bottomMargin: 1 }
                    color: "#acb2c2"
                }
                BorderImage {
                    anchors { fill: parent; leftMargin: 2; topMargin: 5; rightMargin: 1 }
                    border { left: 7; right: 7 }
                    source: "images/tab.png"
                    visible: tabWidget.current == parent.index
                }
                Text {
                    id: label
                    horizontalAlignment: Qt.AlignHCenter; verticalAlignment: Qt.AlignVCenter
                    anchors.fill: parent
                    text: stack.children[parent.index].title
                    elide: Text.ElideRight
                    font.bold: tabWidget.current == parent.index
                }
                TapHandler {
                    onTapped: tabWidget.current = parent.index
                }
            }
        }
    }

    Item {
        id: stack
        width: tabWidget.width
        anchors.top: header.bottom; anchors.bottom: tabWidget.bottom
    }
}
