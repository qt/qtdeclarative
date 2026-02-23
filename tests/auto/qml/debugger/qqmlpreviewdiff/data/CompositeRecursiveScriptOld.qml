// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Old version: child is a plain Item (no composite hierarchy).
import QtQuick

Item {
    property int rootMarker: 1
    Item {
        objectName: "child"
    }
}
