// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Explicit Component: value changed from 10 to 20.
import QtQuick

Item {
    Component {
        id: comp
        Rectangle {
            property int value: 20
        }
    }
    property Component delegate: comp
}
