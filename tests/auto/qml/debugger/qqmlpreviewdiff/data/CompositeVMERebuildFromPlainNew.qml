// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: child becomes an instance of an inline component.
// The instance itself adds no properties — its VME comes from the IC root.
import QtQuick

Item {
    property int rootMarker: 1
    component Inner : Item {
        property int value: 42
    }
    Inner {
        objectName: "child"
    }
}
