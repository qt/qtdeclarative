// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500
    visible: true

    WindowContainer {
        id: foreignWindowContainer
        window: webViewWindow
        anchors.fill: parent
        anchors.margins: 20
    }
}
