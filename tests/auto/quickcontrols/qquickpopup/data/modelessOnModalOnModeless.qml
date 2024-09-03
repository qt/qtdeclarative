// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias modelessPopup: modelessPopup
    property alias button: button
    property alias modalPopup: modalPopup
    property alias tooltip: tooltip

    Popup {
        id: modelessPopup
        modal: false
        closePolicy: Popup.NoAutoClose
        width: 200
        height: 200
        anchors.centerIn: parent
        popupType: Popup.Item

        Button {
            id: button
            checkable: true
            x: 0
            y: 0
            text: "Click me"
        }

        Popup {
            id: modalPopup
            modal: true
            closePolicy: Popup.NoAutoClose
            width: 100
            height: 100
            anchors.centerIn: parent
            popupType: Popup.Item

            Popup {
                id: tooltip
                modal: false
                closePolicy: Popup.NoAutoClose
                width: 50
                height: 50
                anchors.centerIn: parent
                popupType: Popup.Item
            }
        }
    }

}
