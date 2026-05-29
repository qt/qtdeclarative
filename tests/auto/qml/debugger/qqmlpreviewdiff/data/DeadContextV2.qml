// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Rectangle {
    id: root
    width: 200
    height: 200
    color: "red"
    property int counter: 2

    component Inner : Rectangle {
        width: 50
        height: 50
        color: "yellow"
        property int value: 20
    }

    Inner { id: innerInstance }
}
