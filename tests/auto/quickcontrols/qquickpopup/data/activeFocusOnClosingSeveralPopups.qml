// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias button: button
    property alias popup1: popup1
    property alias popup2: popup2

    Button {
        id: button
        focus: true
    }

    Popup {
        id: popup1
        focus: true
        popupType: Popup.Item
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 } }
    }

    Popup {
        id: popup2
        focus: true
        popupType: Popup.Item
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 } }
    }
}
