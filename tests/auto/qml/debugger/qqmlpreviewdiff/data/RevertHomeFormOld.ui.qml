// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Analog of HomeForm.ui.qml before the edit: contains the composite button,
// without setting its "col" (so it keeps the default "grey").
import QtQuick

Item {
    id: home
    property alias theButton: btn
    RevertColorButton {
        id: btn
    }
}
