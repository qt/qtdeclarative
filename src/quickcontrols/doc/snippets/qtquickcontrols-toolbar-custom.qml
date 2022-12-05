// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

//! [file]
ToolBar {
    id: control

    background: Rectangle {
        implicitHeight: 40
        color: "#eeeeee"

        Rectangle {
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
            color: "transparent"
            border.color: "#21be2b"
        }
    }

    RowLayout {
        anchors.fill: parent
        ToolButton {
            text: qsTr("Undo")
        }
        ToolButton {
            text: qsTr("Redo")
        }
    }
}
//! [file]
