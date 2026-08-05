// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

// TODO
QtObject {
    property string customControlName: qsTr("TabBar & TabButton")

    property var supportedStates: [
        ["header", "horizontal"],
        ["header", "horizontal", "disabled"],
        ["footer", "horizontal"],
        ["footer", "horizontal", "disabled"],
        ["header", "vertical"],
        ["header", "vertical", "disabled"],
        ["footer", "vertical"],
        ["footer", "vertical", "disabled"]
    ]

    property Component component: TabBar {
        enabled: !is("disabled")
        position: is("header") ? TabBar.Header : TabBar.Footer
        orientation: is("vertical") ? Qt.Vertical : Qt.Horizontal

        TabButton {
            text: qsTr("TabButton 1")
        }
        TabButton {
            text: qsTr("Icon")
            icon.source: Utils.iconUrl
        }
        TabButton {
            text: qsTr("Transparent Icon")
            icon.source: Utils.iconUrl
            icon.color: "transparent"
        }
        TabButton {
            text: qsTr("TabButton 3")
        }
    }
}
