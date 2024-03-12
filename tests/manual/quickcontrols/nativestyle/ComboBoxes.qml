// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "ComboBoxes"

    Row {
        spacing: container.rowSpacing

        ComboBox {
            model: [ "Default", "Banana", "Apple", "Coconut" ]
        }

        ComboBox {
            model: [ "Disabled", "Banana", "Apple", "Coconut" ]
            enabled: false
        }

        ComboBox {
            model: [ "Small", "Banana", "Apple", "Coconut" ]
            property bool qqc2_style_small
        }

        ComboBox {
            model: [ "Mini", "Banana", "Apple", "Coconut" ]
            property bool qqc2_style_mini
        }
    }

    Row {
        spacing: container.rowSpacing

        ComboBox {
            model: [ "Default", "Banana", "Apple", "Coconut" ]
            editable: true
        }

        ComboBox {
            model: [ "Disabled", "Banana", "Apple", "Coconut" ]
            enabled: false
            editable: true
        }

        ComboBox {
            model: [ "Small", "Banana", "Apple", "Coconut" ]
            editable: true
            property bool qqc2_style_small
        }

        ComboBox {
            model: [ "Mini", "Banana", "Apple", "Coconut" ]
            editable: true
            property bool qqc2_style_mini
        }
    }
}
