// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

// TODO
QtObject {
    property string customControlName: qsTr("TabBar & TabButton")

    property var supportedStates: [
        ["header"],
        ["header", "disabled"],
        ["footer"],
        ["footer", "disabled"]
    ]

    property Component component: TabBar {
        implicitHeight: tabButton1.implicitHeight
        enabled: !is("disabled")
        position: is("header") ? TabBar.Header : TabBar.Footer

        TabButton {
            id: tabButton1
            text: qsTr("TabButton 1")
        }
        TabButton {
            text: qsTr("TabButton 2")
        }
        TabButton {
            text: qsTr("TabButton 3")
        }
    }
}
