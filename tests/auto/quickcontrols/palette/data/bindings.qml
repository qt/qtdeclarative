// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    objectName: "window"
    width: 600
    height: 800
    visible: true

    property alias disabledButton: disabledButton
    property alias enabledButton: enabledButton

    palette {
        active {
            button: "khaki"
            buttonText: "bisque"
        }

        inactive {
            button: "khaki"
            buttonText: "bisque"
        }

        disabled {
            buttonText: "lavender"
            button: "coral"
        }
    }

    ColumnLayout {
        Button {
            id: disabledButton
            text: "Disabled"
            enabled: false

            palette.disabled.button: "aqua"
            palette.disabled.buttonText: "azure"
        }

        Button {
            id: enabledButton
            text: "Enabled"

            palette: disabledButton.palette
        }
    }
}
