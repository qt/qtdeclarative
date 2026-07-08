// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Identical to HotReloadButtonExt/Badge.qml; the patched CalculatorButton needs
// to resolve Badge from its own directory.

import QtQuick

Text {
    property color tint: "white"
    color: tint
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
