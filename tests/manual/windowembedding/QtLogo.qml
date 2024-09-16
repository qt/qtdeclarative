// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

WindowContainer {
    id: logoContainer
    width: 42; height: 30
    window: Window {
        id: videoLogo
        visible: true
        color: "transparent"

        Image {
            source: {
                var logo = "qtlogo-"
                logo += mouseArea.pressed ? "green" : "white"
                logo += ".png"
                return logo
            }
            anchors.fill: parent

            MouseArea {
                id: mouseArea
                anchors.fill: parent
            }
        }
    }
}
