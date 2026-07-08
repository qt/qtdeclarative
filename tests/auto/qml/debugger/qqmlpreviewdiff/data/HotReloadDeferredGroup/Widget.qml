// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A C++ type that defers *everything* except the listed names via
// ImmediatePropertyNames (like Binding/PropertyChanges). "target" is immediate, so
// the id-bearing object can be assigned to it. "sibling.x: base" is then a generalized
// grouped property whose first chain part is the id "sibling", not a property of this
// object. It resolves to no property and is filed in DeferredData under the key -1.
// This is the very reason Binding and PropertyChanges use ImmediatePropertyNames.
// Reloading must re-apply it.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    property int base: 100

    target: Item { id: sibling; objectName: "sibling" }

    sibling.x: base
}
