// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

ControlContainer {
    id: container
    title: "RadioButtons"

    Row {
        spacing: container.rowSpacing

        Column {
            RadioButton {
                text: "Default"
                checked: true
            }
            RadioButton {
                text: "Default"
            }
            RadioButton {
                text: "Default"
            }
        }

        Column {
            RadioButton {
                text: "Disabled"
                enabled: false
            }
            RadioButton {
                text: "Disabled"
                enabled: false
            }
            RadioButton {
                text: "Disabled"
                enabled: false
                checked: true
            }
        }

        Column {
            RadioButton {
                text: "Small"
                property bool qqc2_style_small
            }
            RadioButton {
                text: "Small"
                checked: true
                property bool qqc2_style_small
            }
            RadioButton {
                text: "Small"
                property bool qqc2_style_small
            }
        }

        Column {
            RadioButton {
                text: "Mini"
                property bool qqc2_style_mini
            }
            RadioButton {
                text: "Mini"
                property bool qqc2_style_mini
            }
            RadioButton {
                text: "Mini"
                checked: true
                property bool qqc2_style_mini
            }
        }
    }

}
