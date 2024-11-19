// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic

//! [ListView]
ListView {
    anchors.fill: parent
    model: 3
    delegate: CheckDelegate {
        text: qsTr("Channel %1").arg(index + 1)

        required property int index
        property bool channelActivated

        onClicked: channelActivated = checked
    }
}
//! [ListView]
