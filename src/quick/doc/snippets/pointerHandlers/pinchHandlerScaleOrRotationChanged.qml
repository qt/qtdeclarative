// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 100; height: 100
    color: "lightsteelblue"

    PinchHandler {
        id: handler
        target: null
        onRotationChanged: (delta) => parent.rotation += delta // add
        onScaleChanged: (delta) => parent.scale *= delta // multiply
    }
}
//![0]
