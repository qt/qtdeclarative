// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Pane {
    contentWidth: 120
    contentHeight: 120
    padding: 10
    bottomPadding: 20

//! [1]
Pane {
    width: 120
    height: 120

    Material.elevation: 6

    Label {
        text: qsTr("I'm a card!")
        anchors.centerIn: parent
    }
}
//! [1]
}
