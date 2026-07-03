// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Composite base type. Derived types (in Pad.qml) add their own members on top,
// so their property caches inherit from this one.

import QtQuick

Item {
    property int value: 10
}
