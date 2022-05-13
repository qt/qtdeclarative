// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
        width: 100

        contentItem: ListView {
            implicitHeight: contentHeight
            model: root.model
            delegate: Text { text: modelData }
            clip: true

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}
