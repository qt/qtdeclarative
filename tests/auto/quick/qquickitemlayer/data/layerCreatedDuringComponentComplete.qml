// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    width: 100
    height: 100

    Item {
        objectName: "layered"
        anchors.fill: parent
        onWidthChanged: layer.enabled = true
    }
}
