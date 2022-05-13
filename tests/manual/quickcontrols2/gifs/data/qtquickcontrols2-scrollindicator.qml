// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 100
    height: 120
    visible: true
    color: "#eeeeee"

    ListView {
        anchors.fill: parent

        ScrollIndicator.vertical: ScrollIndicator {
            active: true
        }

        model: 10
        delegate: Label {
            text: qsTr("Item %1").arg(index + 1)
            width: 100
            height: 40
            leftPadding: 10
            verticalAlignment: Text.AlignVCenter
        }
    }
}
