// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Explicit Component: the Component wraps an Item with a property.
import QtQuick

Item {
    Component {
        id: comp
        Rectangle {
            property int value: 10
        }
    }
    property Component delegate: comp
}
