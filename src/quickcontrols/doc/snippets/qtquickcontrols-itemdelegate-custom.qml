// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

ItemDelegate {
    id: control
    text: qsTr("ItemDelegate")

    contentItem: Text {
        rightPadding: control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? (control.down ? "#17a81a" : "#21be2b") : "#bdbebf"
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        color: control.down ? "#dddedf" : "#eeeeee"

        Rectangle {
            width: parent.width
            height: 1
            color: control.down ? "#17a81a" : "#21be2b"
            anchors.bottom: parent.bottom
        }
    }
}
//! [file]
