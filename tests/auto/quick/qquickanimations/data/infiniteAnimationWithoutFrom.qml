// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    width: 100
    height: 100

    // note that 'from' property is missing
    NumberAnimation {
        objectName: "anim"
        to: from+360
        loops: Animation.Infinite
        target: rect
        properties: "rotation"
        duration: 100
    }

    Rectangle {
        objectName: "rect"
        id: rect
        width: 30
        height: 10
        color: "red"
        anchors.centerIn: parent
        transformOrigin: Item.Center
    }
}
