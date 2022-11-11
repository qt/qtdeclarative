// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    property real itemSize: 120 * dp
    property real margin: 10 * dp

    default property alias contents: contentItem.children

    width: contentItem.width + 2 * margin
    height: itemSize + 2 * margin

    Rectangle {
        anchors.fill: parent
        color: "#606060"
        border.color: "#f0f0f0"
        border.width: 1
        opacity: 0.4
    }
    Row {
        id: contentItem
        x: margin
        y: margin
        spacing: margin
    }
}
