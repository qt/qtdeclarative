// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    Rectangle {
        border.color: "red"
        border.width: 4
        width: im1.sourceSize.width + 8
        height: im1.sourceSize.height + 8
        Image {
            id: im1
            source: "qrc:/car_etc2_nomips.ktx"
            anchors.centerIn: parent
        }
    }

    Rectangle {
        anchors.centerIn: parent
        border.color: "red"
        border.width: 4
        width: im2.sourceSize.width + 8
        height: im2.sourceSize.height + 8
        Image {
            id: im2
            source: "qrc:/qt_bc1_10mips.ktx"
            anchors.centerIn: parent
        }
    }
}
