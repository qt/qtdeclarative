// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400

    property alias drawer: drawer
    property alias drawer2: drawer2

    header: Item { implicitHeight: 50 }
    footer: Item { implicitHeight: 50 }

    Drawer {
        id: drawer
        width: parent.width / 2
        implicitHeight: parent.height
    }

    Drawer {
        id: drawer2
        width: Math.min(window.width, window.height) / 3 * 2
        height: window.height
    }
}
