// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: only "tag" changes to trigger a binding diff.
// The states remain identical — the rebuild path will re-evaluate them.

import QtQuick

CompositeContextBaseForm {
    id: derived
    property bool smallMode: false
    property int tag: 2

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
