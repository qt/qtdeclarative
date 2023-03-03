// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//! [0]
DropArea {
    id: dragTarget

    property string colorKey

    width: 64
    height: 64
    keys: [ colorKey ]

    Rectangle {
        id: dropRectangle

        anchors.fill: parent
        color: dragTarget.containsDrag ? "grey" : dragTarget.colorKey
    }
}
//! [0]
