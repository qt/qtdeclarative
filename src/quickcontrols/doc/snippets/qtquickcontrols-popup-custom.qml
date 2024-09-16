// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

Item {
    id: root
    width: 200
    height: 200

    Binding {
        target: popup
        property: "visible"
        value: root.Window.active
    }
//! [1]
Popup {
    id: popup
    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 200
        border.color: "#444"
    }
    contentItem: Column {}
}
//! [1]
}
