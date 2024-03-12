// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.8
import QtQuick.Window 2.2
import QtTest 1.2

TestCase {
    name: "ShortCut"
    when: windowShown

    Shortcut {
        id: shortcut
        property bool everActivated: false
        sequence: StandardKey.NextChild
        onActivated: everActivated = true
    }

    Shortcut {
        id: shortcut2
        property bool everActivated: false
        sequence: "Ctrl+E,Ctrl+W"
        onActivated: everActivated = true
    }

    function test_shortcut() {
        keySequence(StandardKey.NextChild)
        verify(shortcut.everActivated);

        keySequence("Ctrl+E,Ctrl+W")
        verify(shortcut2.everActivated);
    }
}
