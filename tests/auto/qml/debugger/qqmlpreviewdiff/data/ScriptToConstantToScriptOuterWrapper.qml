// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Outer wrapper mimicking ChoosingCoffee.qml.
// Sets cupsLeft from outside the instantiation site (like ChoosingCoffee.qml does).

import QtQuick

ScriptToConstantToScriptWrapper {
    card.cupsLeft: 5
}
