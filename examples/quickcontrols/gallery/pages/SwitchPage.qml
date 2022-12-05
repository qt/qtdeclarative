// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ScrollablePage {
    id: page

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "Switch is an option button that can be dragged or toggled on or off. "
                + "Switches are typically used to select between two states."
        }

        Column {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Switch {
                text: "First"
            }
            Switch {
                text: "Second"
                checked: true
            }
            Switch {
                text: "Third"
                enabled: false
            }
        }
    }
}
