// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property Popup popup: stackView.currentItem.popup

    StackView {
        id: stackView
        objectName: "stackView"
        anchors.fill: parent

        initialItem: Rectangle {
            objectName: "rectangle"

            property alias popup: popup

            Popup {
                id: popup
                objectName: "popup"
                width: 100
                height: 100
                visible: true
                anchors.centerIn: Overlay.overlay
            }
        }
    }
}
