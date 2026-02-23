// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Derived type extending the base form. States are on the root object (not
// a grouped property) so BindingPatchContext doesn't trip on them.
// The PropertyChanges target IDs (header, caption) from the base form's context.
// After rebuild, lookupIdObject will crash if the context hierarchy is wrong.

import QtQuick

CompositeContextBaseForm {
    id: derived
    property bool smallMode: false
    property int tag: 1

    states: [
        State {
            name: "small"
            when: derived.smallMode
            PropertyChanges {
                target: header
                font.pixelSize: 28
            }
            PropertyChanges {
                target: caption
                font.pixelSize: 14
            }
        }
    ]
}
