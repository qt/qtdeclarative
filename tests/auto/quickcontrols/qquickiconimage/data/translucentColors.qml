// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

ApplicationWindow {
    width: 500
    height: 500
    visible: true

    IconImage {
        width: Math.min(250, parent.width)
        height: Math.min(250, parent.height)
        source: "qrc:/icons/testtheme/22x22/actions/color-test-original.png"
        sourceSize: Qt.size(250, 0)
        color: Qt.rgba(0, 0, 0, 0.5)
    }
}
