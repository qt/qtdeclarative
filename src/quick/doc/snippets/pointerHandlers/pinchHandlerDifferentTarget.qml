// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 640
    height: 480

    Rectangle {
        id: map
        color: "aqua"
        width: 400
        height: 300
    }

    PinchHandler {
        target: map
    }
}
//![0]
