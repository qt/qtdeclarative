// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Old version: inline component with one property.
// The instance adds no own properties — its VME comes from the IC root.
import QtQuick

Item {
    property int rootMarker: 1
    component OldInner : Item {
        property int value: 10
        property string label: "old"
    }
    OldInner {
        objectName: "child"
    }
}
