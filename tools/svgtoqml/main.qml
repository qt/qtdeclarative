// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: mainWindow
    width: 1280
    height: 960
    visible: true
    color: "white"

    Item {
        id: svg
        objectName: "svg_item"
        anchors.centerIn: parent
    }
}
