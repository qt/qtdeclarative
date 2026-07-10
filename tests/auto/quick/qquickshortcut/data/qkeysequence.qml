// Copyright (C) 2026 Kai Uwe Broulik <kde@broulik.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Window {
    id: window

    property var activatedShortcut
    property var ambiguousShortcut

    property alias shortcut: shortcut.sequence

    Shortcut {
        id: shortcut
        onActivated: window.activatedShortcut = sequence
        onActivatedAmbiguously: window.ambiguousShortcut = sequence
    }
}
