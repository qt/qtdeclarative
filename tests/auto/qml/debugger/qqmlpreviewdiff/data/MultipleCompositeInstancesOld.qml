// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Multiple instances of composite-with-aliases in the same document.
// Rebuilding the root should rebuild both instances correctly.

import QtQuick

Item {
    width: 400
    height: 400
    property int marker: 1

    CompositeBaseWithAliases {
        id: first
        width: parent.width
        height: parent.height / 2
        header.text: "First"
        header.color: "blue"
    }

    CompositeBaseWithAliases {
        id: second
        y: parent.height / 2
        width: parent.width
        height: parent.height / 2
        header.text: "Second"
        header.color: "green"
    }
}
