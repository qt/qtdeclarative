// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

//! [file]
import QtQuick
import QtQuick.Controls

ScrollBar {
    id: control
    size: 0.3
    position: 0.2
    active: true
    orientation: Qt.Vertical

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 100
        radius: width / 2
        color: control.pressed ? "#81e889" : "#c2f4c6"
        // Hide the ScrollBar when it's not needed.
        opacity: control.policy === ScrollBar.AlwaysOn || (control.active && control.size < 1.0) ? 0.75 : 0

        // Animate the changes in opacity (default duration is 250 ms).
        Behavior on opacity {
            NumberAnimation {}
        }
    }
}
//! [file]
