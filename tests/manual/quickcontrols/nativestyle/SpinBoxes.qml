// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "SpinBoxes"

    Row {
        spacing: container.rowSpacing

        SpinBox {
            value: 500
            to: 2000
            editable: true
        }

        SpinBox {
            value: 0
            enabled: false
        }

        SpinBox {
            value: 5
            from: 0
            to: 9
            property bool qqc2_style_small
        }

        SpinBox {
            value: 0
            from: -9
            to: 9
            property bool qqc2_style_mini
        }
    }

}
