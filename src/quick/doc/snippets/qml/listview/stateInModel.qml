// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic

//! [ListView]
ListView {
    anchors.fill: parent
    model: ListModel {
        ListElement {
            channelActivated: true
        }
        // ...
    }
    delegate: CheckDelegate {
        text: qsTr("Channel %1").arg(index + 1)
        checked: model.channelActivated

        required property int index
        required property var model

        onClicked: model.channelActivated = checked
    }
}
//! [ListView]
