// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Dials"

    Row {
        spacing: container.rowSpacing

        Dial {
            width: 50
            height: 50
            from: 0
            to: 10
            value: 5
        }

        Dial {
            width: 50
            height: 50
            from: 0
            to: 10
            value: 5
            stepSize: 1
            property int qqc2_style_tickPosition: 1
        }
    }
}
