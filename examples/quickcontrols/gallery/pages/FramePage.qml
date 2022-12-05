// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ScrollablePage {
    id: page

    readonly property int itemWidth: Math.max(button.implicitWidth, Math.min(button.implicitWidth * 3, page.availableWidth / 3 * 2))

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "Frame is used to layout a logical group of controls together, within a visual frame."
        }

        Frame {
            anchors.horizontalCenter: parent.horizontalCenter

            Column {
                spacing: 20
                width: page.itemWidth

                RadioButton {
                    text: "First"
                    checked: true
                    width: parent.width
                }
                RadioButton {
                    id: button
                    text: "Second"
                    width: parent.width
                }
                RadioButton {
                    text: "Third"
                    width: parent.width
                }
            }
        }
    }
}
