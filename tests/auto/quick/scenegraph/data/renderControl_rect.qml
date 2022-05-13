// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Rectangle {
    width: 200
    height: 200
    color: "steelblue"
    Rectangle {
        width: 150
        height: 150
        anchors.centerIn: parent
        color: "red"
    }
}
