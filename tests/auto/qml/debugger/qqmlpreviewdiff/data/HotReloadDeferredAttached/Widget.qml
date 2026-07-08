// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type deriving a C++ type that defers *everything* via
// ImmediatePropertyNames (like Binding/PropertyChanges). The DeferredAttached.amount
// binding is therefore a deferred attached-property binding, which resolves to no
// property on the object and is filed in DeferredData under the key -1. Reloading
// must re-apply it.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    property int base: 100
    DeferredAttached.amount: base
}
