// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import ".."

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500

    WindowContainer {
        id: foreignWindowContainer
        window: mapWindow
        anchors.fill: parent
        anchors.margins: 20

        QtLogo {
            anchors {
                top: parent.top
                left: parent.left
                margins: 10
            }
        }
    }
}
