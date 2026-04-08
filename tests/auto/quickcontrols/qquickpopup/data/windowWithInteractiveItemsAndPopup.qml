// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 320
    height: 240
    Row {
        spacing: 6
        Button {
            id: popupBtn
            text: "show popup"
            onClicked: {
                myPopup.visible = true
            }
        }
        Button {
            objectName: "extra button"
            text: "click me"
        }
        Rectangle {
            id: rect
            objectName: "tappable rect"
            color: th.pressed ? "steelblue" : "#00FF00"
            width: 30
            height: 30
            TapHandler { id: th }
        }
    }

    Popup {
        id: myPopup
        anchors.centerIn: parent
        visible: false
        modal: true
        popupType: Popup.Native
        closePolicy: Popup.NoAutoClose
        width: 240
        height: 160
        dim: true
        Button {
            text: "close"
            onClicked: myPopup.close()
        }
    }
}
