// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    visible: true
    width: menuBar.implicitWidth
    height: menuBar.height

    Component.onCompleted: menuBar.itemAt(1).highlighted = true

    header:

// Indent it like this so that the indenting in the generated doc is normal.
MenuBar {
    id: menuBar

    Menu { title: qsTr("File") }
    Menu { title: qsTr("Edit") }
    Menu { title: qsTr("View") }
    Menu { title: qsTr("Help") }

    delegate: MenuBarItem {
        id: menuBarItem

        contentItem: Text {
            text: menuBarItem.text
            font: menuBarItem.font
            opacity: enabled ? 1.0 : 0.3
            color: menuBarItem.highlighted ? "#ffffff" : "#21be2b"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitWidth: 40
            implicitHeight: 40
            opacity: enabled ? 1 : 0.3
            color: menuBarItem.highlighted ? "#21be2b" : "transparent"
        }
    }

    background: Rectangle {
        implicitWidth: 40
        implicitHeight: 40
        color: "#ffffff"

        Rectangle {
            color: "#21be2b"
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
        }
    }
}
} //! [eof]
