// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup1: popup1
    property alias popup2: popup2
    property alias closePopup2Button: closePopup2Button

    Popup {
        id: popup1
        focus: true
    }

    Popup {
        id: popup2
        focus: true

        Button {
            id: closePopup2Button
            onClicked: {
                popup1.contentItem.forceActiveFocus();
                popup2.close();
            }
        }
    }
}
