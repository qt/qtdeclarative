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
        width: 256
        height: 256
        color: "green"
    }

    Rectangle {
        x: 64
        y: 172
        width: 256
        height: 256
        color: "blue"
    }
}
//![0]
