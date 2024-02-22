// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 400
    height: 400

    property alias popup: popup
    property int model: 40

    Popup {
        id: popup
        height: 100

        contentItem: ListView {
            implicitWidth: contentWidth
            model: root.model
            delegate: Text { text: modelData }
            clip: true
            orientation: Qt.Horizontal

            ScrollIndicator.horizontal: ScrollIndicator {}
        }
    }
}
