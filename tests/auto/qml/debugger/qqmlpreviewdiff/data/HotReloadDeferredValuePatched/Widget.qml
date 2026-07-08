// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadDeferredValue/Widget.qml but base changed from 100 to 200.

import QtQuick
import Qt.Test.PreviewDeferred

DeferredIntItem {
    property int base: 200
    amount: base
}
