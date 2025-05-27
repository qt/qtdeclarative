// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    id: root

    width: 320
    height: 480

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
    }

    Rectangle {
        id: rectangle
        x: 40
        y: 20
        width: 120
        height: 120
        color: "red"

        TapHandler {
            onTapped: rectangle.width += 10
        }
    }
}
//![0]
