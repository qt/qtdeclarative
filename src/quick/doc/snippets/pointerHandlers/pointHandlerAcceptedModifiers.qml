// Copyright (C) 2023 Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    id: feedbackPane
    width: 480; height: 320

    PointHandler {
        id: control
        acceptedModifiers: Qt.ControlModifier
        cursorShape: Qt.PointingHandCursor
        target: Rectangle {
            parent: feedbackPane
            color: control.active ? "indianred" : "khaki"
            x: control.point.position.x - width / 2
            y: control.point.position.y - height / 2
            width: 20; height: width; radius: width / 2
        }
    }

    PointHandler {
        id: shift
        acceptedModifiers: Qt.ShiftModifier | Qt.MetaModifier
        cursorShape: Qt.CrossCursor
        target: Rectangle {
            parent: feedbackPane
            color: shift.active ? "darkslateblue" : "lightseagreen"
            x: shift.point.position.x - width / 2
            y: shift.point.position.y - height / 2
            width: 30; height: width; radius: width / 2
        }
    }
}
//![0]
