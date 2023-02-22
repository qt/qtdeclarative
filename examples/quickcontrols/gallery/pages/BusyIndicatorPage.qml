// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

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
            text: "BusyIndicator is used to indicate activity while content is being loaded,"
                  + " or when the UI is blocked waiting for a resource to become available."
        }

        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
