// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Examples

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample(qsTr("Tiles"), qsTr("Press and drag tiles to move them into the matching colored boxes"),  Qt.resolvedUrl("tiles/tiles.qml"))
            addExample(qsTr("GridView"), qsTr("Press and drag to re-order items in the grid"), Qt.resolvedUrl("views/gridview.qml"))
            addExample(qsTr("External"), qsTr("Drag and drop between this and other applications."), Qt.resolvedUrl("external/externaldraganddrop.qml"))
        }
    }
}
