// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadGroupIdClash/Widget.qml but spot.x changed from 100 to 200. The patch
// must land on the host's "spot" value-type property; the object with id "spot" (and its own
// x) must be left untouched.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    target: Item { id: spot; objectName: "spotItem"; x: 7 }

    spot.x: 200
}
