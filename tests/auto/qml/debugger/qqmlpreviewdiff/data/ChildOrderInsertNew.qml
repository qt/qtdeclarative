// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Three children. The "beta" child is new — inserted between existing ones.
// Object index 2 (was "gamma") becomes "beta"; a new object @3 carries "gamma".
import QtQuick

Item {
    Item { objectName: "alpha"; width: 10 }
    Item { objectName: "beta"; y: 20 }
    Item { objectName: "gamma"; x: 30 }
}
