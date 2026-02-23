// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Implicit Component wrapper: value changed from 10 to 20.
import QtQuick

Item {
    property Component delegate: Rectangle {
        property int value: 20
    }
}
