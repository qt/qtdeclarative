// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick 2.12

Item {
    width: 640
    height: 480

    PinchHandler {
        id: handler
        target: null
    }

    Text {
        color: handler.active ? "darkgreen" : "black"
        text: handler.rotation.toFixed(1) + " degrees\n" +
              handler.translation.x.toFixed(1) + ", " + handler.translation.y.toFixed(1) + "\n" +
              (handler.scale * 100).toFixed(1) + "%"
    }
}
//![0]
