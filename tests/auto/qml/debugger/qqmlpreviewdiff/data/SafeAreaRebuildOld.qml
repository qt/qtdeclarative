// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Minimal reproducer for SafeArea assertion crash after rebuild.
// The root Item uses SafeArea attached property, which registers a
// Matrix change listener. After rebuild, the attached object is
// orphaned but still registered, causing assertion failure on resize.

import QtQuick

Item {
    id: root
    width: 200
    height: 200

    property int safeLeft: SafeArea.margins.left

    Rectangle {
        id: background
        anchors.fill: parent
        color: "blue"
    }
}
