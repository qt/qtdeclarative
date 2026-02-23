// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as Old but status changed.

import QtQuick

CompositeBaseWithAliases {
    id: mainItem
    width: 400
    height: 400
    header.text: "Main"

    Item {
        parent: mainItem.content
        width: parent ? parent.width : 0
        height: 50

        Text {
            id: innerLabel
            text: "Inner: " + mainItem.header.text
            anchors.centerIn: parent
        }
    }

    property string status: "new"
}
