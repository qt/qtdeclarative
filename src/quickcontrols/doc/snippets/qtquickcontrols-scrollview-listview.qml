// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 200
    height: 200

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

    ListView {
        model: 20
        delegate: ItemDelegate {
            text: "Item " + index

            required property int index
        }
    }
}
//! [file]
}
