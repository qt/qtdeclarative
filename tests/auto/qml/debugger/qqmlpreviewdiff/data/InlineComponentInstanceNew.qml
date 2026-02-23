// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Inline component with an instantiated instance.
// New version: Inner.value changed to 20.
import QtQuick

Item {
    component Inner : Item {
        property int value: 20
    }
    Inner {
        objectName: "inner"
    }
    property int rootMarker: 1
}
