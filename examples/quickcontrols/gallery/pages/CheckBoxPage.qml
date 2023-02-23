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
            text: qsTr("CheckBox presents an option button that can be toggled on or off. "
                + "Check boxes are typically used to select one or more options from a set of options.")
        }

        Column {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            CheckBox {
                text: qsTr("First")
                checked: true
            }
            CheckBox {
                text: qsTr("Second")
            }
            CheckBox {
                text: qsTr("Third")
                checked: true
                enabled: false
            }
        }
    }
}
