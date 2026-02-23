// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Modified version: background color changed from "blue" to "red".

import QtQuick

Item {
    id: root
    width: 200
    height: 200

    property int safeLeft: SafeArea.margins.left

    Rectangle {
        id: background
        anchors.fill: parent
        color: "red"
    }
}
