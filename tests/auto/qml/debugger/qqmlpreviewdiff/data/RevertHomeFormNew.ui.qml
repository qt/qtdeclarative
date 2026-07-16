// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Analog of HomeForm.ui.qml after removing "col: green" again (identical to Old).
import QtQuick

Item {
    id: home
    property alias theButton: btn
    RevertColorButton {
        id: btn
    }
}
