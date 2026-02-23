// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Base form type with IDs that are referenced by the derived type's state bindings.

import QtQuick

Item {
    objectName: "baseForm"
    id: root
    property alias header: headerText
    property alias caption: captionText
    property alias grid: grid

    Item {
        objectName: "grid"
        id: grid
        anchors.fill: parent

        Text {
            objectName: "header"
            id: headerText
            text: "Title"
            font.pixelSize: 32
        }
        Text {
            objectName: "caption"
            id: captionText
            text: "Subtitle"
            font.pixelSize: 16
        }
    }
}
