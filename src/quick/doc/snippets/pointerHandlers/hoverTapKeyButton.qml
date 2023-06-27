// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    id: button
    signal clicked

    width: 150; height: 50; radius: 3
    color: tapHandler.pressed ? "goldenrod" : hoverHandler.hovered ? "wheat" : "beige"
    border.color: activeFocus ? "brown" : "transparent"
    focus: true

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        id: tapHandler
        onTapped: button.clicked()
    }

    Keys.onEnterPressed: button.clicked()
}
//![0]
