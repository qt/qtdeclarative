// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.12

Item {
    width: 300
    height: 200
    property var taps: []

    Rectangle {
        x: 25
        y: 25
        width: 200
        height: 100
        color: "salmon"

        TapHandler {
            objectName: "parentTapHandler"
            onTapped: taps.push(objectName)
        }

        Rectangle {
            x: 25
            y: 25
            width: 200
            height: 100
            color: "lightsteelblue"

            TapHandler {
                objectName: "childTapHandler"
                onTapped: taps.push(objectName)
            }
        }
    }
}
