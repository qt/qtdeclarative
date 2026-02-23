// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite base type with internal IDs referenced by property aliases.
// Simulates ApplicationFlowForm.ui.qml from the coffee example.

import QtQuick

Item {
    id: root

    property alias content: contentArea
    property alias header: headerText

    Text {
        id: headerText
        text: "Header"
        anchors.top: parent.top
    }

    Item {
        id: contentArea
        anchors.top: headerText.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
}
