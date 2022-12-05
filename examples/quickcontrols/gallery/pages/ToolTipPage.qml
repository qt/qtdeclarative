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
            text: "A tool tip is a short piece of text that informs the user of a control's function."
        }

        Button {
            text: "Tip"
            anchors.horizontalCenter: parent.horizontalCenter

            ToolTip.timeout: 5000
            ToolTip.visible: pressed
            ToolTip.text: "This is a tool tip."
        }
    }
}
