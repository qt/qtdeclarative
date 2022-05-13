// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9

Flickable {
    width: 200; height: 200
    contentWidth: rect.width; contentHeight: rect.height

    property real contentPosAdjustment: 0.0

    onContentXChanged: {
        var adjustment = contentPosAdjustment
        contentPosAdjustment = 0.0
        contentX += adjustment
    }

    onContentYChanged: {
        var adjustment = contentPosAdjustment
        contentPosAdjustment = 0.0
        contentY += adjustment
    }

    Rectangle {
        id: rect
        border.color: "red"
        border.width: 5
        width: 400; height: 400
    }
}
