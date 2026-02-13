// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    property alias popup: popup
    property alias childWindow: window2
    id: window1
    width: 500
    height: 500

    Text {
        anchors.centerIn: parent
        text: "window1"
    }

    Popup {
        id: popup
        popupType: Popup.Window
        width: 400
        height: 400
        background: Rectangle {
            color: "green"
        }

        Text {
            anchors.centerIn: parent
            text: "popup x " + popup.x + " y " + popup.y
        }
    }

    Window {
        id: window2
        transientParent: window1
        x: 500
        y: 100
        width: 500
        height: 500

        Text {
            anchors.centerIn: parent
            text: "window2"
        }
    }
}

