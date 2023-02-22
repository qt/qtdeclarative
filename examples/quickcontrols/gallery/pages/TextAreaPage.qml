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
            text: "TextArea is a multi-line text editor."
        }

        TextArea {
            width: page.availableWidth / 3
            anchors.horizontalCenter: parent.horizontalCenter

            wrapMode: TextArea.Wrap
            text: "TextArea\n...\n...\n..."
        }
    }
}
