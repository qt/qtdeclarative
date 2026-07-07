// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    component Inner: Rectangle {
        color: "blue"
        property int marker: 1
    }
    Inner { objectName: "inner" }
}
