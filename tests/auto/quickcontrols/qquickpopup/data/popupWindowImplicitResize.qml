// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 600
    title: qsTr("Popup explicit-size bug repro")

    property alias popup: popup
    property alias button: button

    Popup {
        id: popup
        width: 300
        height: 300
        Rectangle {
            id: rectangle
            readonly property int size: button.checked ? 500 : 300
            implicitWidth: size
            implicitHeight: size
            anchors.fill: parent
            color: "grey"
            Label {
                text: `Dialog.size: (${popup.width}, ${popup.height})
Dialog child item's implicitSize: (${rectangle.implicitWidth}, ${rectangle.implicitHeight})`
            }
            Button {
                id: button
                text: checked ? qsTr(">Resize<") : qsTr("< Resize >")
                checkable: true
                anchors.centerIn: parent
            }
        }
    }
}
