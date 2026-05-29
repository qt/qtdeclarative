// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Rectangle {
    id: root
    width: 200
    height: 200
    color: "blue"
    property int counter: 1

    component Inner : Rectangle {
        width: 50
        height: 50
        color: "green"
        property int value: 10
    }

    Inner { id: innerInstance }
}
