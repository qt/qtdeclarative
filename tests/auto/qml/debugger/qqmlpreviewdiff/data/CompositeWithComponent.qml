// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Composite base type with a Component{} child (explicit component).
// The Component creates an implicit sub-context.

import QtQuick

Item {
    id: root

    property alias delegate: delegateComponent
    property alias label: labelText

    Text {
        id: labelText
        text: "Default"
    }

    Component {
        id: delegateComponent
        Rectangle {
            width: root.width
            height: 30
            color: "lightgray"
            Text {
                text: labelText.text
                anchors.centerIn: parent
            }
        }
    }
}
