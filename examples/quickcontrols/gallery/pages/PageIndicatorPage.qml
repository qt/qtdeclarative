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
            text: "PageIndicator is used to indicate the currently active page in a container of pages."
        }

        PageIndicator {
            count: 5
            currentIndex: 2
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
