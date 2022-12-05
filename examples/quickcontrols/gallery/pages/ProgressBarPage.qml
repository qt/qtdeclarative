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
            text: "ProgressBar indicates the progress of an operation. It can be set in an "
                + "indeterminate mode to indicate that the length of the operation is unknown."
        }

        ProgressBar {
            id: bar
            value: 0.5
            anchors.horizontalCenter: parent.horizontalCenter
        }

        ProgressBar {
            indeterminate: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
