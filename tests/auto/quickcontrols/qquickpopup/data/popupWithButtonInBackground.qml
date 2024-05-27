// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 1080
    height: 720

    property alias popup: simplepopup

    Button {
        text: "Button"
    }

    Popup {
        id: simplepopup
        popupType: Popup.Window

        x: 50
        y: 50

        Text {
            text: "I am a very interesting popup"
        }
    }
}
