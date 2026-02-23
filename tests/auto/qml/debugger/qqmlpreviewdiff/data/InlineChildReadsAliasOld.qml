// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Instance with an inline Component that creates items placed in the
// composite base's alias property (content). Tests context scoping
// across inline component boundaries during rebuild.

import QtQuick

CompositeBaseWithAliases {
    id: mainItem
    width: 400
    height: 400
    header.text: "Main"

    // Assign children to the 'content' alias target
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

    property string status: "old"
}
