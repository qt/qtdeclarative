// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 50; height: 50
    color: "black"
    Text {
        color: "white"
        text: String.fromCharCode(65 + Math.floor(26*Math.random()))
        anchors.centerIn: parent
    }
}
