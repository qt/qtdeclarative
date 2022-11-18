// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 200
    height: 200
    border.color: "#ddd"

    Binding {
        target: root.children[0].ScrollBar.horizontal
        property: "active"
        value: true
    }

    Binding {
        target: root.children[0].ScrollBar.vertical
        property: "active"
        value: true
    }

//! [file]
ScrollView {
    width: 200
    height: 200

    Label {
        text: "ABC"
        font.pixelSize: 224
    }
}
//! [file]
}
