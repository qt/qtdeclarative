// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// ImmediateHost defers everything except the listed names (like Binding/PropertyChanges).
// "self.px: base" is a generalized grouped property whose first chain part is the id "self"
// of the host itself. It lands in DeferredData under the key -1. The host survives the
// reload (rebuilt in place) and px/py are C++ properties, so their values persist across
// the rebuild.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    id: self
    property int base: 100

    self.px: base
}
