// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 200
    height: 200

    property bool receivedKeyPress: false

    Item {
        objectName: "item"
        focus: true
        anchors.fill: parent

        Keys.onLeftPressed: receivedKeyPress = true
    }
}

