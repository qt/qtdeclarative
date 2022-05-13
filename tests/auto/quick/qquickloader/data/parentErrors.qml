// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 360
    height: 360

    property alias loader: loader

    Loader {
        id: loader
        anchors.fill: parent
    }

    property Component component: Rectangle {
        width: parent.width
        height: parent.height
        color: "pink"
    }
}
