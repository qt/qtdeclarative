// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "CheckBoxes"

    Row {
        spacing: container.rowSpacing

        CheckBox {
            text: "Default"
            checked: true
        }

        CheckBox {
            text: "Disabled"
            enabled: false
        }

        CheckBox {
            text: "Tri-state"
            tristate: true
            checkState: Qt.PartiallyChecked
        }

        CheckBox {
            text: "Small"
            property bool qqc2_style_small
        }

        CheckBox {
            text: "Mini"
            property bool qqc2_style_mini
            checked: true
        }
    }
}
