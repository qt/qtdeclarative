// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400

    property alias popup: popup
    property alias nestedPopup: nestedPopup
    property alias popupSlider: popupSlider
    property alias contentSlider: contentSlider

    Slider {
        id: contentSlider
        anchors.fill: parent
        wheelEnabled: true
    }

    Popup {
        id: popup
        x: 50; y: 50
        implicitWidth: parent.width - 100
        implicitHeight: parent.height - 100
        clip: true
        contentItem: Slider {
            id: popupSlider
            wheelEnabled: true
        }

        Popup {
            id: nestedPopup
            x: 0; y: 0
            clip: true
            implicitWidth: 50
            implicitHeight: 50
        }
    }
}
