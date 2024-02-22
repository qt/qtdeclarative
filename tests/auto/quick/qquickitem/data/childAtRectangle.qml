// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    width: 20
    height: 20

    Rectangle {
        id: rect1

        color: "red"
        x: 0
        y: 0
        width: 16
        height: 16
    }

    Rectangle {
        id: rect2

        color: "red"
        x: 18
        y: 0
        width: 1
        height: 1
    }

    Rectangle {
        id: rect3

        x: 19
        y: 19
        width: 0
        height: 0
    }

}

