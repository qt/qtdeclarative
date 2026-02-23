// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Grandparent composite type with a script binding (computed property).
import QtQuick

Item {
    property int gpValue: 7
    property int gpDoubled: gpValue * 2
}
