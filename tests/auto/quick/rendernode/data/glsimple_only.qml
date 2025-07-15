// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Test

Item {
    width: 100
    height: 100
    GLSimpleItem {
        anchors.fill: parent
        Rectangle {
            color: "gray"
            width: 50
            height: 50
            anchors.centerIn: parent
        }
    }
}
