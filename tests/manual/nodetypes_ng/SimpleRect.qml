// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Item {
    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "red"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }

        Rectangle {
            color: "gray"
            width: 50
            height: 50
            anchors.centerIn: parent
        }
    }
}
