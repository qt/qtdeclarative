// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Uses a composite type with states. Starts in "active" state.

import QtQuick

Item {
    width: 300
    height: 300

    CompositeWithStates {
        id: stateful
        width: parent.width
        height: parent.height
        state: "active"
    }

    property string tag: "old"
}
