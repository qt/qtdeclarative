// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Label {
    id: root
    fontSizeMode: Label.Fit
    horizontalAlignment: Label.AlignHCenter
    verticalAlignment: Label.AlignVCenter

    Material.foreground: Material.theme === Material.Light
        ? Material.color(Material.Indigo, !dim ? Material.Shade500 : Material.Shade100)
        : Material.color(Material.Indigo, dim ? Material.Shade300 : Material.Shade100)

    Layout.fillHeight: true

    property bool dim: false
    property alias interactive: tapHandler.enabled

    signal tapped

    TapHandler {
        id: tapHandler
        onTapped: root.tapped()
    }
}
