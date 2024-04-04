// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

//! [1]
ToolBar {
    RowLayout {
        anchors.fill: parent

        ToolButton {
            text: qsTr("Action 1")
        }
        ToolButton {
            text: qsTr("Action 2")
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Action 3")
        }
        ToolButton {
            text: qsTr("Action 4")
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Action 5")
        }
        ToolButton {
            text: qsTr("Action 6")
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
//! [1]
