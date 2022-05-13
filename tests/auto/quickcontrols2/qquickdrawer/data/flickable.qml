// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias drawer: drawer
    property alias flickable: flickable

    Drawer {
        id: drawer
        width: 300
        height: 400

        contentItem: ListView {
            id: flickable
            model: 100
            delegate: ItemDelegate {
                width: flickable.width
                text: index
            }
        }
    }
}
