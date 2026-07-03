// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadRemovedBaseProp/BaseWidget.qml, but the "gone" property has
// been removed. Derived instances that still carry a binding targeting "gone"
// must have that binding-target entry cleared during the relink, not left
// pointing at the stale (old-layout) property data.

import QtQuick

Item {
    property int stay: 2
}
