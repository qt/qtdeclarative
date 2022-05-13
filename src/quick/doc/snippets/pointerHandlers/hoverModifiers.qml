// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 150; height: 50; radius: 3
    color: control.hovered ? "goldenrod" : shift.hovered ? "wheat" : "beige"

    HoverHandler {
        id: control
        acceptedModifiers: Qt.ControlModifier
        cursorShape: Qt.PointingHandCursor
    }

    HoverHandler {
        id: shift
        acceptedModifiers: Qt.ShiftModifier
        cursorShape: Qt.CrossCursor
    }
}
//![0]
