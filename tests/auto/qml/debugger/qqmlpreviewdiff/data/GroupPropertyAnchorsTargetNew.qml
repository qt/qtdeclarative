// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 200; height: 200

    Rectangle {
        id: sibling
        width: 100; height: 100
    }

    Rectangle {
        id: target
        anchors.fill: sibling
    }
}
