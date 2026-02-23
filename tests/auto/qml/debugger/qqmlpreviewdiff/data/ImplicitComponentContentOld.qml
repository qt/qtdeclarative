// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Implicit Component wrapper: the Rectangle is wrapped in an automatic
// Component object by the compiler.
import QtQuick

Item {
    property Component delegate: Rectangle {
        property int value: 10
    }
}
