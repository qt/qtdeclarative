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
            text: "RadioButton presents an option button that can be toggled on or off. "
                + "Radio buttons are typically used to select one option from a set of options."
        }

        Column {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            RadioButton {
                text: "First"
            }
            RadioButton {
                text: "Second"
                checked: true
            }
            RadioButton {
                text: "Third"
                enabled: false
            }
        }
    }
}
