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
            text: "SpinBox allows the user to choose an integer value by clicking the up or down indicator buttons, "
                + "by pressing up or down on the keyboard, or by entering a text value in the input field."
        }

        SpinBox {
            id: box
            value: 50
            anchors.horizontalCenter: parent.horizontalCenter
            editable: true
        }
    }
}
