// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "TextFields"

    Row {
        spacing: container.rowSpacing

        TextField {
            text: "Default"
        }

        TextField {
            enabled: false
            text: "Disabled"
        }

        TextField {
            placeholderText: "Placeholder text"
        }

        TextField {
            text: "Small"
            property bool qqc2_style_small
        }

        TextField {
            text: "Mini"
            property bool qqc2_style_mini
        }
    }
}
