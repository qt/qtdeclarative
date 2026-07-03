// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Composite base type. Derived instances (in Pad.qml) bind to both of its
// properties. On reload one of them (gone) is removed.

import QtQuick

Item {
    property int gone: 1
    property int stay: 2
}
