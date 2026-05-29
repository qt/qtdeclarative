// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Inner wrapper mimicking ChoosingCoffeeForm.ui.qml.
// Declares the card but does NOT set cupsLeft (that's set by the outer wrapper).

import QtQuick

Item {
    id: innerRoot
    width: 300
    height: 300
    property alias card: card

    ScriptToConstantToScriptDerived {
        id: card
        width: parent.width
        height: parent.height
    }
}
