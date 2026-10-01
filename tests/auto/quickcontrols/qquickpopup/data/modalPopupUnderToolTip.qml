// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias backgroundMouseArea: backgroundMouseArea
    property alias modalPopup: modalPopup
    property alias toolTip: toolTip

    MouseArea {
        id: backgroundMouseArea
        anchors.fill: parent
    }

    Item {
        id: toolTipParent
        width: 50
        height: 50
    }

    ToolTip {
        id: toolTip
        parent: toolTipParent
        text: "tip"
    }

    Popup {
        id: modalPopup
        modal: true
        closePolicy: Popup.NoAutoClose
        popupType: Popup.Item
        x: 100
        y: 100
        width: 200
        height: 200
    }
}
