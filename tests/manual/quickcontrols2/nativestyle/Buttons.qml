// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Buttons"

    Row {
        spacing: container.rowSpacing

        Button {
            text: "Default"
        }

        Button {
            text: "Disabled"
            enabled: false
        }

        Button {
            text: "Small"
            property bool qqc2_style_small
        }

        Button {
            text: "Mini"
            property bool qqc2_style_mini
        }
    }

    Row {
        spacing: container.rowSpacing

        Button {
            text: "Explicit height"
            height: 50
        }

        Button {
            text: "Explicit width"
            width: 200
        }
    }

    Row {
        spacing: container.rowSpacing

        Button {
            text: "Highlighted"
            highlighted: true
        }

        Button {
            text: "Flat"
            flat: true
        }

        Button {
            text: "Checkable"
            checkable: true
        }
    }
}
