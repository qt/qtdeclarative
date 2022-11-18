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
            text: "DelayButton is a checkable button that incorporates a delay before the "
                + "button is activated. This delay prevents accidental presses."
        }

        DelayButton {
            text: "DelayButton"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
