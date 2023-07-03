// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Rectangle {
    color: "#272822"
    width: 320
    height: 480

    Rectangle {
        y: 64
        z: 1
        width: 256
        height: 256
        color: "green"

        Rectangle {
            x: 192
            y: 64
            z: 2000
            width: 128
            height: 128
            color: "red"
        }
    }

    Rectangle {
        x: 64
        y: 192
        z: 2
        width: 256
        height: 256
        color: "blue"
    }
}
//![0]
