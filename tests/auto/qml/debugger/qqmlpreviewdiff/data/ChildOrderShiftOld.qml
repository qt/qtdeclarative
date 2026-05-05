// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Two children in the default property. Adding/removing a property on the
// root will shift the binding indices of the children.
import QtQuick

Item {
    Item { objectName: "alpha"; width: 10 }
    Item { objectName: "beta"; height: 20 }
}
