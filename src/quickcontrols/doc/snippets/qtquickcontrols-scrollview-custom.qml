// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic

Item {
    width: 200
    height: 200

    Binding {
        target: control.ScrollBar.horizontal
        property: "active"
        value: true
    }

    Binding {
        target: control.ScrollBar.vertical
        property: "active"
        value: true
    }

//! [file]
ScrollView {
    id: control

    width: 200
    height: 200
    focus: true

    Label {
        text: "ABC"
        font.pixelSize: 224
    }

    ScrollBar.vertical: ScrollBar {
        parent: control
        x: control.mirrored ? 0 : control.width - width
        y: control.topPadding
        height: control.availableHeight
        active: control.ScrollBar.horizontal.active
    }

    ScrollBar.horizontal: ScrollBar {
        parent: control
        x: control.leftPadding
        y: control.height - height
        width: control.availableWidth
        active: control.ScrollBar.vertical.active
    }

    background: Rectangle {
        border.color: control.activeFocus ? "#21be2b" : "#bdbebf"
    }
}
//! [file]
}
