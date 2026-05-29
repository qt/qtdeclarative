// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Like ScriptToConstantToScriptOuterWrapper.qml but uses a script expression
// (2 + 3) instead of a literal (5) to set cupsLeft from outside.

import QtQuick

ScriptToConstantToScriptWrapper {
    card.cupsLeft: 2 + 3
}
