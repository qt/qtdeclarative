// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T // we cannot use QQC2 (yet), but we can use its backend

T.Slider {
    id: control

    // QQC2-specific begin
    implicitWidth: Math.max(control.implicitBackgroundWidth + control.leftInset
                            + control.rightInset,
                            control.implicitHandleWidth + control.leftPadding
                            + control.rightPadding)
    implicitHeight: Math.max(control.implicitBackgroundHeight + control.topInset
                             + control.bottomInset,
                             control.implicitHandleHeight + control.topPadding
                             + control.bottomPadding)
    padding: 6
    // QQC2-specific end

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 4
        width: control.availableWidth
        height: implicitHeight
        radius: 2
        border.color: "black"
        color: "#F9F3EC"

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: "#63ACBE"
            radius: 2
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 26
        implicitHeight: 26
        radius: 13
        color: control.pressed ? Qt.lighter("#63ACBE") : "#63ACBE"
        border.color: Qt.darker("#63ACBE")
    }
}
