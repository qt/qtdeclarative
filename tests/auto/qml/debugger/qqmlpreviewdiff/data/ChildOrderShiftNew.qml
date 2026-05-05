// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same two children, but a property initializer pushes the data bindings
// to higher indices in the binding table (shift right).
import QtQuick

Item {
    property int marker: 42
    Item { objectName: "alpha"; width: 10 }
    Item { objectName: "beta"; height: 20 }
}
