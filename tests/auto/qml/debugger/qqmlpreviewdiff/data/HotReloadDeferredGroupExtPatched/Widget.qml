// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadDeferredGroupExt/Widget.qml, but the generalized grouped property's
// left-hand side changed from self.px to self.py (and base 100 -> 200). Because the host
// survives the reload, the old self.px must be reset; otherwise it keeps its stale value
// (100). The extra child forces a full rebuild rather than an in-place patch.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    id: self
    property int base: 200

    self.py: base

    Item { objectName: "extra" }
}
