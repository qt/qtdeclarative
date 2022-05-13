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
            addExample("Tiles", "Press and drag tiles to move them into the matching colored boxes",  Qt.resolvedUrl("tiles/tiles.qml"));
            addExample("GridView", "Press and drag to re-order items in the grid", Qt.resolvedUrl("views/gridview.qml"));
        }
    }
}
