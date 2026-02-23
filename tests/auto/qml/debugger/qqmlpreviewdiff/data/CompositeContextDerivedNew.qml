// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: the "when" condition is changed (added width check).
// This triggers a binding change diff, causing rebuild.

import QtQuick

CompositeContextBaseForm {
    objectName: "derivedNew"
    property bool smallMode: false

    grid.states: [
        State {
            objectName: "smallState"
            name: "small"
            when: smallMode && width > 0
            PropertyChanges {
                objectName: "headerChanges"
                target: header
                font.pixelSize: 28
            }
            PropertyChanges {
                objectName: "captionChanges"
                target: caption
                font.pixelSize: 14
            }
        }
    ]
}
