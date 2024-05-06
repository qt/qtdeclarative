// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias topLevelComboBox: topLevelComboBox
    property alias popup: popup
    property alias comboBoxInPopup: comboBoxInPopup

    ComboBox {
        id: topLevelComboBox
        model: ["ONE", "TWO", "THREE"]
    }

    Popup {
        id: popup
        width: 200
        height: 200
        visible: true
        palette.window: "red"

        ComboBox {
            id: comboBoxInPopup
            model: ["ONE", "TWO", "THREE"]
        }
    }
}
