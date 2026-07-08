// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type in its own .qml file, used as the deferred contentItem of
// CalculatorButton. It is neither an inline component nor a base type of the
// patched type — it is the *content* pulled in from an extra file.

import QtQuick

Text {
    property color tint: "white"
    color: tint
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
