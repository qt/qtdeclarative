// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type deriving a C++ type with a deferred *value-type* property
// (amount: int). The deferred binding assigns a value; it does not create an
// object. Reloading must re-apply the deferred value binding.

import QtQuick
import Qt.Test.PreviewDeferred

DeferredIntItem {
    property int base: 100
    amount: base
}
