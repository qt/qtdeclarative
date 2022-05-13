// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import "methods.js" as Utils


Item {
    height: 480
    width: 320

    property alias msg: ttext

    Label { id: ttext; anchors.fill: parent; anchors.margins: 10 }

    Button {
        id: button
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.bottom
        anchors.margins: 10
        antialiasing: true

        text: qsTr("Request data.xml")

        onClicked: Utils.makeRequest()
    }
}
