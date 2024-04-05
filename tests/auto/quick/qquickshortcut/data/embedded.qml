// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    width: 300
    height: 300

    property bool activated: false
    property alias shortcut: shortcut

    Shortcut {
        id: shortcut
        onActivated: root.activated = true
    }
}
