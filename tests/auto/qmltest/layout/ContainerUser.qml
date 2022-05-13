// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.6
import QtQuick.Window 2.2

Container {
    visible: true

    Text {
        objectName: "qtbug51927-text"
        text: qsTr("Hello World")
        anchors.centerIn: parent
        renderType: Text.QtRendering
    }
}
