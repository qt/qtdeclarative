// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400
    title: "dimmerContainmentMask"

    property alias modalPopup: modalPopup
    property int clickCount: 0

    MouseArea {
        anchors.fill: parent
        onClicked: ++clickCount;
    }

    Popup {
        id: modalPopup
        modal: true
        x: 100
        y: 100
        width: 200
        height: 200
    }
}
