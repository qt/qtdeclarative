// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// New version: inline component definition changed (renamed, property added, value changed).
// The instance still adds no own properties — its VME comes from the IC root.
import QtQuick

Item {
    property int rootMarker: 1
    component NewInner : Item {
        property int value: 20
        property string label: "new"
        property real extra: 3.14
    }
    NewInner {
        objectName: "child"
    }
}
