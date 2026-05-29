// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 200
    height: 200

    property alias listView: listView

    ListView {
        id: listView
        anchors.fill: parent
        model: 1
        delegate: Item {
            property alias button: button
            Popup {
                id: popup
                contentItem: Button {
                    id: button
                    text: "Test"
                }
            }
            Component.onCompleted: { popup.open() }
        }
    }
}
