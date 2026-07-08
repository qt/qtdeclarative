// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadDeferredAttached/Widget.qml but base changed from 100 to 200
// and an extra child object added. The child addition is a structural change that
// forces a full rebuild (rebuildObject -> recompleteDeferred) rather than an
// in-place patch, so the deferred attached-property binding must be re-armed.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    property int base: 200
    DeferredAttached.amount: base

    Item { objectName: "extra" }
}
