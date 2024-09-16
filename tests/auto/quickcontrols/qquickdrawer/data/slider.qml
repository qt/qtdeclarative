// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias drawer: drawer
    property alias slider: slider

    Drawer {
        id: drawer
        width: 300
        height: 400
        position: 1.0
        visible: true

        Slider {
            id: slider
            value: 1
            width: parent.width
        }
    }
}
