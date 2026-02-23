// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Derived type extending the base form. Has a state with a "when" binding
// that references IDs (header, caption) from the base form's context.
// This mimics the coffee demo's Home.qml extending HomeForm.ui.qml.

import QtQuick

CompositeContextBaseForm {
    objectName: "derivedOld"
    property bool smallMode: false

    grid.states: [
        State {
            objectName: "smallState"
            name: "small"
            when: smallMode
            PropertyChanges {
                objectName: "headerChanges"
                target: header
                font.pixelSize: 28
            }
            PropertyChanges {
                objectname: "captionChanges"
                target: caption
                font.pixelSize: 14
            }
        }
    ]
}
