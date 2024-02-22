// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        []
    ]

    property Component component: SplitView {
        implicitWidth: 400
        implicitHeight: 100

        Rectangle {
            color: "salmon"
            implicitWidth: 25
            implicitHeight: 25
        }
        Rectangle {
            color: "navajowhite"
            implicitWidth: 100
            implicitHeight: 100
        }
        Rectangle {
            color: "steelblue"
            implicitWidth: 200
            implicitHeight: 200
        }
    }
}
