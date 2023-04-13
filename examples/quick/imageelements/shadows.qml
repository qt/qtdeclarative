// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: window

    width: 480
    height: 320
    color: "gray"

    ShadowRectangle {
        anchors.centerIn: parent
        width: 250
        height: 250
        color: "lightsteelblue"
    }

    ShadowRectangle {
        anchors.centerIn: parent
        width: 200
        height: 200
        color: "steelblue"
    }

    ShadowRectangle {
        anchors.centerIn: parent
        width: 150
        height: 150
        color: "thistle"
    }
}
