// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadDeferredGroup/Widget.qml but base changed from 100 to 200 and an
// extra child object added. The child addition is a structural change that forces a
// full rebuild (rebuildObject -> recompleteDeferred) rather than an in-place patch,
// so the deferred generalized-grouped-property binding must be re-armed.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    property int base: 200

    target: Item { id: sibling; objectName: "sibling" }

    sibling.x: base

    Item { objectName: "extra" }
}
