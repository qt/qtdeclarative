// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

SwipeDelegate {
    id: control
    text: qsTr("SwipeDelegate")

    Component {
        id: component

        Rectangle {
            color: SwipeDelegate.pressed ? "#333" : "#444"
            width: parent.width
            height: parent.height
            clip: true

            Label {
                text: qsTr("Press me!")
                color: "#21be2b"
                anchors.centerIn: parent
            }
        }
    }

    swipe.left: component
    swipe.right: component

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? (control.down ? "#17a81a" : "#21be2b") : "#bdbebf"
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter

        Behavior on x {
            enabled: !control.down
            NumberAnimation {
                easing.type: Easing.InOutCubic
                duration: 400
            }
        }
    }
}
//! [file]
