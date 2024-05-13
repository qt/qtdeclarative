// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.Basic

//! [file]
ToolBar {
    RowLayout {
        anchors.fill: parent

        ToolButton {
            text: qsTr("Action 1")
        }
        ToolButton {
            text: qsTr("Action 2")
        }

        ToolSeparator {
            padding: vertical ? 10 : 2
            topPadding: vertical ? 2 : 10
            bottomPadding: vertical ? 2 : 10

            contentItem: Rectangle {
                implicitWidth: parent.vertical ? 1 : 24
                implicitHeight: parent.vertical ? 24 : 1
                color: "#c3c3c3"
            }
        }

        ToolButton {
            text: qsTr("Action 3")
        }
        ToolButton {
            text: qsTr("Action 4")
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
//! [file]
