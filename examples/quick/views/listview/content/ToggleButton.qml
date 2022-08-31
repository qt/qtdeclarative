// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    property alias label: text.text
    property bool active: false
    signal toggled
    width: 149
    height: 30
    radius: 3
    color: active ? "green" : "lightgray"
    border.width: 1
    Text { id: text; anchors.centerIn: parent; font.pixelSize: 14 }
    MouseArea {
        anchors.fill: parent
        onClicked: { root.active = !root.active; root.toggled() }
    }
}
