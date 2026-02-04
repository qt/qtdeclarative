// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: root
    color: "black"
    width: 320
    height: 320

    property bool itemVisible: true
    property string itemColor: "red"

    Rectangle {
        anchors.fill: parent
        visible: root.itemVisible
        color: root.itemColor
    }
}
